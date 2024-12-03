#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(const char *path, const char *filename)
{
    char buf[512], *p
    int fd;
    struct dirent de;
    struct stat st;
    if((fd = open(path, 0)) < 0)
    {
        fprintf(2, "ls: cannot open %s\n", path);
        exit(1);
    }
    if(fstat(fd, &st) < 0)
    {
        fprintf(2, "ls: cannot stat %s\n", path);
        eixt(1);
    }
    switch(st.type)
    {
        case T_FILE:
            fprintf(2, "Usage: find dir file\n");
			exit(1);
        break;
        case T_DIR:
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
            {
                printf("ls: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf+strlen(buf);
            *p++ = '/';
            while(read(fd, &de, sizeof(de)) == sizeof(de))
            {
                if(de.inum == 0 || strcmp(de.name, ".") ==0 || strcmp(de.name, "..") == 0)
                    continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                if(stat(buf, &st) < 0)
                {
                    printf("ls: cannot stat %s\n", buf);
                    continue;
                }
                if(st.type == T_DIR)
                    find(buf, filename);
                else if(st.type == T_FILE)
                    if(strcmp(de.name, filename) == 0)
                        printf("%s\n", buf);
            }
        break;
    }
}

int main(int argc, int *argv[])
{
    if(argc != 3)
    {
        sprintf(2, "find argc not 3\n");
        eixt(1);
    }
    const char *path = argv[1];
    const char *filename = argv[2];
    find(path, filename);
}