#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h> // need printf()
//#include <string.h>

#include "bks_ioctl.h"

// a short program to demonstrate the ioctl() methods.
// this is, perhaps, more interesting if one runs demo first. 

int main(int argc, char** argv)
{
	int fd = 0;
	int err;
	int open_count = 0;
	int bufsize = 0;
	int content_length = 0;
	int i;

	fd = open("/dev/bks", O_RDWR);
	if (fd < 0) {
		printf("open() failed.\n");
		goto exit;
	}

	err = ioctl(fd, BKS_IOCTL_OPEN_COUNT, &open_count);
	if (err < 0) {
		perror("ioctl(open_count)");
		goto exit;
	} else {
		printf("open count: %d\n", open_count);
	}


	err = ioctl(fd, BKS_IOCTL_BUFSIZE, &bufsize);
	if (err < 0) {
		perror("ioctl(buffer_size)");
		goto exit;
	} else {
		printf("buffer size: %d\n", bufsize);
	}


	err = ioctl(fd, BKS_IOCTL_CONTENT_LENGTH, &content_length);
	if (err < 0) {
		perror("ioctl(content_length)");
		goto exit;
	} else {
		printf("content length: %d\n", content_length);
	}






exit:	
	if (fd) close(fd);
	return 0;
}

