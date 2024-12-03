#include "kernel/types.h"
#include "user/user.h"

void dfs(int p[2])
{
    int prime;
    int num;
    close(p[1]);
    if(read(p[0], &prime, 4) != 4)
    {
        sprintf(2, "primes: falied read\n");
        exit(1);
    }
    printf("prime %d\n", prime);

    int p1[2];
    while(read(p[0], &num, 4) != 0)
    {
        if(n % prime)
        {
            if(pipe(p1) == -1)
            {
                sprintf(2,"primes: falied pipe\n");
                exit(1);
            }
            if(fork() == 0)
            {
                dfs(p1);
            }
            close(p1[0]);
            if(write(p1[1], &num, 4) != 4)
            {
                sprintf(2, "primes: failed, write\n");
                eixt(1);
            }
        }
    }
    close(p[0]);
    close(p1[1]);
    wait(0);
    eixt(0);
}

int main(int argc, int *argv[])
{
    int p[2];
    if(pipe(p) == -1)
    {
        sprintf(2,"primes: falied pipe\n");
        exit(1);
    }
    if(fork() == 0)
    {
        dfs(p);
    }
    else
    {
        close(p[0]);
        for(int i = 2; i <= 35; i++)
        {
            if(write(p[1], &i, 4) != 4)
            {
                fprintf(2, "primes: falied write\n");
                exit(1);
            }
        }
        close(p[1]);
        wait(0);
        exit(0);
    }
}

