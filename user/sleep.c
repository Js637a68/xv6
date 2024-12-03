#include "kernel/types.h"
#include "user/user.h"

int main(int argc, int *argv[])
{
    if(argc != 2)
    {
        fprintf(2, "sleep: argc != 2\n");
        exit(1);
    }
    int t = atoi(argv[1]);
    sleep(t);
    exit(0);
}