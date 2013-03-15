#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>

#define BUFSIZE 4096

int main(int argc, char** argv)
{
    int i, fd, len, wlen;
    int nr = 0;
    char* mptr;
    size_t bufsize = BUFSIZE;
    char* buffer = malloc(bufsize);

    if (!buffer) {
        printf("malloc() failed. bufsize: %d\n", bufsize);
        goto out;
    }
    if (argc > 1) {
        nr = atoi(argv[1]);
    }

    if (nr) {
        fd = open("/dev/bks_mmap1", O_RDWR | O_SYNC);
    } else {
        fd = open("/dev/bks_mmap0", O_RDWR | O_SYNC);
    }
    if (fd < 0) {
        perror("open()");
        goto outfree;
    }

    /**
      * Requesting mmaping  at offset 0 on device memory, last argument.
      * This is used by driver to map device memory. First argument is the
      * virtual memory location in user address space where application
      * wants to setup the mmaping. Normally, it is set to 0  to allow
      * kernel to pick the best location in user address space. Please
      * review man pages: mmap(2)
      */

    mptr = mmap(0, bufsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mptr == MAP_FAILED) {
        perror("mmap()");
        goto outclose;
    }

    /**
      * Now mmap memory region can be access as user memory. No syscall
      * overhead!
      */

    /* read from mmap memory */
    printf("mptr is %p\n", mptr);
    memset(buffer, 0, bufsize);
    memcpy(buffer, mptr, bufsize-1);
    printf("mmap:  '%s'\n", buffer);
    printf("mptr is %p\n", mptr);

    /* write to mmap memory */
    memcpy(mptr, "MY TEST STRING!", 16);
    memset(buffer, 0, bufsize);
    memcpy(buffer, mptr, bufsize-1);
    printf("mmap:  '%s'\n", buffer);

    munmap(mptr, bufsize);
outclose:
    close(fd);
outfree:
    free(buffer);
out:
    return 0;
}
