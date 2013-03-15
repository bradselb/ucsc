#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/errno.h>

char* message = "now is the time for all good men to come to the aid of their country";

int main(int argc, char** argv)
{
    int rc = 0;
    int n;
    int fd;

    char* buf = 0;
    int cursor = 0;
    int len = 0;
    int msglen = strlen(message);

    if (1 < argc) {
        message = argv[1];
        msglen = strlen(message);
    }

    fd = open("/dev/poll_dev", O_WRONLY);
    if (fd < 0) {
        perror("open");
        rc = -1;
        goto exit;
    }

    while (1) {
        buf = &message[cursor];
        len = strlen(buf);
        n = write(fd, buf, len);
        if (n < 0) {
            perror("write");
            rc = -1;
            break;
        } else if (0 == strncmp(buf, "quit", 4)) {
            break;
        } else {
            // wrote some...maybe not all.
            printf("wrote %d bytes.\n", n);
            //printf("%s\n", buf);
            cursor = (cursor + n) % msglen;
        }
        sleep(2);
    }

exit:
    return rc;
}

