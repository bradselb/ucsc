#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memset()
#include <errno.h> // perror()
#include <unistd.h> //read(), write()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <sys/ioctl.h>

static const char* G_DeviceName = "/dev/bks";

int main(int argc, char **argv)
{
	int rc = 0;
	int fd = -1;
	long result;
	size_t bufsize = 512; // do not exceed 4096
	char* buf = 0;
	ssize_t count = 0;
	// do reads in chunks of this many bytes each
	// to see that the library calls the driver repeatedly.
	int chunk_size = 16; // must NOT be greater than bufsize.

	// allocate a buffer
	buf = malloc(bufsize);
	if (buf) {
		//printf("allocated a buffer of size, %lu\n", bufsize);
	} else {
		printf("failed to allocate buffer of size, %lu\n", bufsize);
		goto exit;
	}

	// open the device file.
	fd = open(G_DeviceName, O_RDWR);
	printf("fd: %d\n", fd);
	if (fd < 0) {
		perror("open device file failed\n");
		goto exit;
	}

	// write a string to the page allocated by bksbuf
	memset(buf, 0, bufsize);
	strncpy(buf, "now is the time for all good men to come to the aid of their country.\n", bufsize-1); 
	count = write(fd, buf, strnlen(buf,bufsize));

	// read the string a chunk at a time and print it out
	memset(buf, 0, bufsize);
	while (0 < (count = read(fd, buf, chunk_size))) {
		printf("%s", buf);
		fflush(stdout);
		memset(buf, 0, chunk_size); 
	}
	fflush(stdout);

	// write a string to the page allocated by bksbuf
	memset(buf, 0, bufsize);
	strncpy(buf, "this is another line of pithy text\n", bufsize-1); 
	count = write(fd, buf, strnlen(buf,bufsize));

	// read the string a chunk at a time and print it out
	memset(buf, 0, bufsize);
	while (0 < (count = read(fd, buf, chunk_size))) {
		printf("%s", buf);
		fflush(stdout);
		memset(buf, 0, chunk_size); 
	}
	fflush(stdout);

	sleep(15);

exit:
	if (0 < fd) close(fd);
	if (buf) free(buf);

	return rc;
}


