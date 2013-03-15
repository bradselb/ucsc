#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/errno.h>

#define EXPIRE	100 /*poll time out */
#define BUFSIZE 40
int main(void)
{
    struct pollfd fds[2];
    int ret, rfd, wfd;
    int len;
    char  rbuf[BUFSIZE];
    const char* wbuf = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int total_bytes = strlen(wbuf);
    int count;

    rfd = open("/dev/poll_dev", O_RDONLY);
    if (rfd < 0) {
        perror("open for read");
        return -1;
    }
    fds[0].fd = rfd;
    fds[0].events = POLLIN;

    wfd = open("/dev/poll_dev", O_WRONLY);
    if (wfd < 0) {
        perror("open for write");
        return -1;
    }
    fds[1].fd = wfd;
    fds[1].events = POLLOUT;

    while (1) {
        ret = poll(fds, 2, EXPIRE * 1000);
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

            /* Device returns maximum of 23 bytes, size of the device */
            ret = read(rfd, rbuf, total_bytes);
            if (ret == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }
            printf("\nREAD %d byte: %s\n", ret, rbuf);
            if (ret < total_bytes) {
                printf("\nNeeds to read addtional %d bytes\n", (total_bytes - ret));
                total_bytes -= ret;
            } else {
                printf("\nALL DATA IS READ. Exiting ...\n");
                break;
            }
        }

        if (fds[1].revents & POLLOUT) {
            printf("fd is writable\n");
            /* Capacity of the device is limited to 23 bytes */
            ret = write(wfd, wbuf, strlen(wbuf));
            if (ret == -1) {
                perror("write");
                break;
            }

            printf("\n WROTE %d byte|size of buffer %d\n", ret, strlen(wbuf));
            if (ret < strlen(wbuf)) {
                printf("\nNeeds to write addtional %d bytes\n", (strlen(wbuf) - ret));
                wbuf += ret;
            } else {
                printf("\n All data is written\n");
            }
        }
    }//while

    close(rfd);
    close(wfd);
    exit(EXIT_SUCCESS);

}
