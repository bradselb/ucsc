#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "bks_ioctl.h"

int main(int argc, char** argv)
{
    int pattern = 0;
    int fd = 0;

    fd = open("/dev/bks_blink0", O_RDWR);
    if (fd < 0) {
        perror("open");
        goto exit;
    }


    if (1 < argc) {
        // assume cmd line arg is the new pattern number.
        pattern = atoi(argv[1]);
        ioctl(fd, BKS_IOCTL_SET_BLINK_PATTERN, &pattern);
    } else {
        // this is a query. 
        ioctl(fd, BKS_IOCTL_GET_BLINK_PATTERN, &pattern);
        printf("the current blink pattern is %d\n", pattern);
    }

    close(fd);


exit:
    return 0;
}

