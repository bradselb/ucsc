#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/errno.h>


int main(int argc, char** argv)
{
    int rc = 0;
    int n;
    int fd;

    int bufsize = 32;
    char* buf = 0;


    if (1 < argc) {
        n = atoi(argv[1]);
        if (0 < n) {
            bufsize = n;
        }
    }

    buf = malloc(bufsize);

    fd = open("/dev/poll_dev", O_RDONLY);
    if (fd < 0) {
        perror("open");
        rc = -1;
        goto exit;
    }

    while (1) {
        memset(buf, 0, bufsize);
        n = read(fd, buf, bufsize-1);// buf is always null terminated.
        if (n < 0) {
            perror("read");
            rc = -1;
            break;
        } else {
            printf("read %d bytes:\t'%s'\n", n, buf);
        }
        if (0 == strncmp(buf, "quit", 4)) {
            break;
        }
    }

exit:
    if (buf) {
        free(buf);
    }
    return rc;
}

