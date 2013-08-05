#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memset()
#include <errno.h> // perror()
#include <unistd.h> //read(), write()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "kbuffer.h"

int main(int argc, char **argv)
{
	int rc = 0;
	int fd = -1;
	long result;
	size_t bufsize = 512;
	char* buf = 0;
	ssize_t count = 0;
	int data;

	// allocate a buffer
	buf = malloc(bufsize);
	if (buf) {
		//printf("allocated a buffer of size, %lu\n", bufsize);
	} else {
		printf("failed to allocate buffer of size, %lu\n", bufsize);
		goto exit;
	}

	// open the device file.
	fd = open("/dev/bks", O_RDWR);
	if (fd < 0) {
		perror("open() failed\n");
		goto exit;
	}

	// ask for the string length
	result = ioctl(fd, KBUFFER_GET_STRING_LENGTH, 0);
	if (0 < result) {
		printf("the length of the string in the buffer is: %ld\n", result);
	}
	else {
		printf("ioctl(fd, KBUFFER_GET_STRING_LENGTH) returned: %ld\n", result);
	}

	// read the string and print it out
	memset(buf, 0, bufsize);
	count = read(fd, buf, bufsize-1);
	printf("%s\n", buf);

	// write a different string
	memset(buf, 0, bufsize);
	strncpy(buf, "String updated", bufsize-1); 
	count = write(fd, buf, 1+strnlen(buf,bufsize));

	// find the length of the new string. pass in a pointer this time
	result = ioctl(fd, KBUFFER_GET_STRING_LENGTH, &data);
	if (0 == result) {
		printf("the length of the string in the buffer is: %ld\n", data);
	}
	else {
		printf("ioctl(fd, KBUFFER_GET_STRING_LENGTH) returned: %ld\n", result);
	}

	// read the string and print it out again
	memset(buf, 0, bufsize);
	count = read(fd, buf, bufsize-1);
	printf("%s\n", buf);

	// re-initialize the string
	result = ioctl(fd, KBUFFER_REINITIALIZE, 0 );
	printf("ioctl(fd, KBUFFER_REINITIALIZE ) returned: %ld\n", result);

	// read the string and print it out again
	memset(buf, 0, bufsize);
	count = read(fd, buf, bufsize-1);
	printf("%s\n", buf);

	// resize the buffer.
	result = ioctl(fd, KBUFFER_RESIZE, 512 );
	printf("ioctl(fd, KBUFFER_RESIZE ) returned: %ld\n", result);

	struct kbuffer_ctx ctx;
	ctx.foo = 17; // the solution to the universe.
	ctx.bar = 42; // the answer is...42!
	result = ioctl(fd, KBUFFER_UPDATE_CTX, &ctx);
	printf("ioctl(fd, KBUFFER_UPDATE_CTX ) returned: %ld\n", result);

	fflush(stdout);

exit:
	if (0 < fd) close(fd);
	if (buf) free(buf);

	return rc;
}


