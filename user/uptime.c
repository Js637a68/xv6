#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    uint64 ticks = uptime();
    printf("%d\n", ticks);
    exit(0);
}