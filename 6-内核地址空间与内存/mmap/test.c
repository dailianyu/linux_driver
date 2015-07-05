
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>

#define MMAP_LENGTH 4096

int
main(int argc, char **argv)
{
    int fd;
    void *p = NULL;

    fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        perror("failed to open file");
        return -1;
    }

    p = mmap(NULL, MMAP_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
    if (!p) {
        perror("failed to mmap file");
        return -1;
    }

    //memset(p, 0, MMAP_LENGTH);
    //memcpy(p, "12345", 5);
    printf("%s\n",p);

    munmap(p, MMAP_LENGTH);
    close(fd);

    return 0;

}
