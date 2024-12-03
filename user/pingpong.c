#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int main(int argc, char *argv[])
{
    char buf[2];
    int p1[2],p2[2];
    pipe(p1);
    pipe(p2);
    int pid;
    if(fork() == 0)
    {
        close(p1[1]);
        close(p2[0]);
        if(read(p1[0], buf, sizeof(buf)) <=0)
        {
            fprintf(2, "child failed read ping\n");
            exit(1);
        }
        pid = getpid();
        printf("%d: received ping\n", pid);
        if(write(p2[1], "o", 1) < 1)
        {
            fprintf(2, "child  failed write pong\n");
            exit(1);
        }
        close(p1[0]);
        close(p2[1]);
        exit(0);
    }
    close(p1[0]);
    close(p2[1]);
    if(write(p1[1], "i", 1) < 1)
    {
        fprintf(2, "parent failed write ping\n");
        exit(1);
    }
    if(read(p2[0], buf, 1) <= 0)
    {
        fprintf(2, "parent failed read pong\n");
        exit(1);
    }
    pid = getpid();
    printf("%d: received pong\n", pid);
    close(p1[1]);
    close(p2[0]);
    exit(0);
}