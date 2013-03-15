#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/errno.h>

#define EXPIRE	100 /*poll time out */
#define BUFSIZE 23
int main(void)
{
    struct pollfd fds[1];
    int ret, rfd;
    int len;
    char rbuf[BUFSIZE];
    int count;

    rfd = open("/dev/poll_dev", O_RDONLY);
    fds[0].fd = rfd;
    fds[0].events = POLLIN;
    /* blocking for event on fd for 100 seconds */
    ret = poll(fds, 1, EXPIRE * 1000);
    if (ret == -1) {
        perror("poll");
        return -1;
    }
    if (!ret) {
        printf("%d seconds elapsed, \n", EXPIRE);
        return 0;
    }

    if (fds[0].revents & POLLIN) {
        printf("fd is readable\n");
        memset(rbuf,0,BUFSIZE);
        ret = read(rfd, rbuf, BUFSIZE);
        printf("\nREAD %d byte: %s\n", ret, rbuf);
        if (ret == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        printf("\nREAD %d byte: %s\n", ret, rbuf);

    }
    close(rfd);
    exit(EXIT_SUCCESS);
}
