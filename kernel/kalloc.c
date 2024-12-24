// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct ref_struct {
  struct spinlock lock;
  int refcnt[PHYSTOP/PGSIZE];
} ref;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&ref.lock, "ref");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  acquire(&ref.lock);
  if(--ref.refcnt[(uint64)pa/PGSIZE] < 1) {
    release(&ref.lock);
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  } else {
    release(&ref.lock);
  }
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk
    acquire(&ref.lock);
    ref.refcnt[(uint64)r/PGSIZE] = 1;
    release(&ref.lock);
  }
  return (void*)r;
}


void addrefcnt(uint64 pa)
{
  acquire(&ref.lock);
  ++ref.refcnt[pa/PGSIZE];
  release(&ref.lock);
}

int refcnt(uint64 pa)
{
  return ref.refcnt[pa/PGSIZE];
}

int cowpage(pagetable_t pagetable, uint64 va)
{
  pte_t *pte;
  if(va >= MAXVA || 
    (pte = walk(pagetable, va, 0)) == 0 || (*pte & PTE_RSW) == 0) return -1;
  return 0;
}

void* cowalloc(pagetable_t pagetable, uint64 va)
{
  pte_t *pte = walk(pagetable, va, 0);
  uint64 pa = PTE2PA(*pte);
  

  if(ref.refcnt[pa / PGSIZE] < 2) {
    *pte = (*pte | PTE_W) & ~PTE_RSW;
    return (void*)pa;
  } else{
    char* mem = kalloc();
    memmove(mem, (char*)pa, PGSIZE);
    int flags = (PTE_FLAGS(*pte) & ~PTE_RSW) | PTE_W ;
    *pte = PA2PTE((uint64)mem) | flags;
    
    kfree((void*)pa);
    return (void*)mem;
  }  
}
