// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13
const uint UINT_MAX = -1;

struct spinlock tick_lock;
uint ttt;

struct hashbuf {
  struct spinlock lock;
  struct buf head;  // 指向桶中的第一个buf
};

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct hashbuf buckets[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&tick_lock, "time");
  initlock(&bcache.lock, "bcache");
  for(int i = 0; i < NBUCKET; ++i)
  {
    char name[10];
    snprintf(name, sizeof(name), "bcache_%d", i);
    initlock(&bcache.buckets[i].lock, name);

    bcache.buckets[i].head.prev = &bcache.buckets[i].head;
    bcache.buckets[i].head.next = &bcache.buckets[i].head;
  }


  // Create linked list of buffers
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    int hash = (b - bcache.buf) % NBUCKET;
    struct hashbuf *hb = &bcache.buckets[hash];
    b->next = hb->head.next;
    b->prev = &hb->head;
    initsleeplock(&b->lock, "buffer");
    hb->head.next->prev = b;
    hb->head.next = b;
  }
}

int tick()
{
  int ret;
  acquire(&tick_lock);
  ret = ++ttt;
  release(&tick_lock);
  return ret;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf* nocached(uint dev, uint blockno)
{
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  struct buf *b, *lru = 0;
  uint min = UINT_MAX;
  int hash = -1;

  acquire(&bcache.lock);
  for(int i = 0; i < NBUCKET; ++i)
  {
    acquire(&bcache.buckets[i].lock);
    for(b = bcache.buckets[i].head.prev; b != &bcache.buckets[i].head; b = b->prev){
      if(b->refcnt == 0 && b->ticks < min) {
        min = b->ticks;
        lru = b;
        hash = i;
      }
    }
    release(&bcache.buckets[i].lock);
  }

  if (lru && hash != -1) {
    acquire(&bcache.buckets[hash].lock);
    // 从桶中移除lru
    lru->next->prev = lru->prev;
    lru->prev->next = lru->next;
    release(&bcache.buckets[hash].lock);
    // 移入新桶
    hash = (dev+blockno) % NBUCKET;
    acquire(&bcache.buckets[hash].lock);
    lru->next = bcache.buckets[hash].head.next;
    lru->prev = &bcache.buckets[hash].head;
    bcache.buckets[hash].head.next->prev = lru;
    bcache.buckets[hash].head.next = lru;

    lru->dev = dev;
    lru->blockno = blockno;
    lru->valid = 0;
    lru->refcnt = 1;
    lru->ticks = tick();  // 更新时间戳
    release(&bcache.buckets[hash].lock);
    release(&bcache.lock);
    acquiresleep(&lru->lock);
    return lru;
  }

  panic("bget: no buffers");
}

static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int hash = (dev+blockno) % NBUCKET;

  acquire(&bcache.buckets[hash].lock);

  // Is the block already cached?
  for(b = bcache.buckets[hash].head.next; b != &bcache.buckets[hash].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      b->ticks = tick();
      release(&bcache.buckets[hash].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.buckets[hash].lock);

  return nocached(dev, blockno);
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int hash = (b->dev+b->blockno) % NBUCKET;
  acquire(&bcache.buckets[hash].lock);
  b->refcnt--;
  b->ticks = tick();
  
  release(&bcache.buckets[hash].lock);
}

void
bpin(struct buf *b) {
  int hash = (b->dev+b->blockno) % NBUCKET;
  acquire(&bcache.buckets[hash].lock);
  b->refcnt++;
  release(&bcache.buckets[hash].lock);
}

void
bunpin(struct buf *b) {
  int hash = (b->dev+b->blockno) % NBUCKET;
  acquire(&bcache.buckets[hash].lock);
  b->refcnt--;
  release(&bcache.buckets[hash].lock);
}
