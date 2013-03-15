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
    int freq = 0;
    int fd = 0;

    fd = open("/dev/bks_blink0", O_RDWR);
    if (fd < 0) {
        perror("open");
        goto exit;
    }


    if (1 < argc) {
        // user wants to set the blink frequency.
        freq = atoi(argv[1]);
        ioctl(fd, BKS_IOCTL_SET_BLINK_FREQUENCY, &freq);
    } else {
        // this is a query. 
        ioctl(fd, BKS_IOCTL_GET_BLINK_FREQUENCY, &freq);
        printf("the current blink frequency is %d (Hz)\n", freq);
    }

    close(fd);


exit:
    return 0;
}
