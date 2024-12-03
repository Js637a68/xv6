#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int readline(char *new_argv[MAXARG], int argc)
{
    char buf[1024];
    int n = 0;
    while(read(0, buf+n, 1))
    {
        if(n==1023) exit(1);
        if(buf[n] == '\n') break;
        n++;
    }
    buf[n] = 0;
    if(n==0) return 0;
    int offset = 0;
    while(offset < n)
    {
        new_argv[argc++] = buf + offset;
        while(buf[offset] != ' ' && offset < n) offset++;
        while(buf[offset] == ' ') buf[offset++] = 0;
    }
    new_argv[argc] = 0;
    return n;
}

int main(int argc, char *argv[])
{
    if(argc <= 1)
    {
        fprintf(2, "xargs: command(arg...)\n");
        exit(1);
    }
    char *command = argv[1];
    char *new_argv[MAXARG];
    for(int i = 1; i < argc; ++i)
    {
        new_argv[i-1] = malloc(strlen(argv[i])+1);
        strcpy(new_argv[i-1], argv[i]);
    }
    while(readline(new_argv, argc-1))
    {

        if(fork() == 0)
        {
            exec(command, new_argv);
            exit(1);
        }
        wait(0);
    }
    for(int i = 1; i < argc; ++i)
        free(new_argv[i-1]);
    exit(0);
}