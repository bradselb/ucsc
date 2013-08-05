#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memset()
#include <errno.h> // perror()
#include <unistd.h> //read(), write()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
//#include <unistd.h>

#include "bksbuf.h"

int main(int argc, char **argv)
{
	int rc = 0;
	int fd_bksbuf = -1;
	int fd_devmem = -1;
	long result;
	size_t bufsize = 512; // do not exceed 4096
	char* buf = 0;
	ssize_t count = 0;

	// allocate a buffer
	buf = malloc(bufsize);
	if (buf) {
		//printf("allocated a buffer of size, %lu\n", bufsize);
	} else {
		printf("failed to allocate buffer of size, %lu\n", bufsize);
		goto exit;
	}

	// open the device file.
	fd_bksbuf = open("/dev/bksbuf", O_RDWR);
	if (fd_bksbuf < 0) {
		perror("open(\"/dev/bksbuf\") failed\n");
		goto exit;
	}


	// write a string to the page allocated by bksbuf
	memset(buf, 0, bufsize);
	strncpy(buf, "now is the time for all good men to come to the aid of their country.", bufsize-1); 
	count = write(fd_bksbuf, buf, 1+strnlen(buf,bufsize));

	// read the string and print it out
	memset(buf, 0, bufsize); 
	count = read(fd_bksbuf, buf, bufsize-1);
	printf("%s\n", buf);

	// ask for the address of the kernel buffer. 
	struct bksbuf_addr addr = { 0, 0};
	result = ioctl(fd_bksbuf, BKSBUF_GET_ADDRESS, &addr);
	//printf("ioctl(fd_bksbuf, BKSBUF_GET_ADDRESS) returned: %ld\n", result);
	printf("bksbuf virtual: 0x%08lx,  physical: 0x%08lx\n", addr.virt, addr.phys);
	fflush(stdout);

	// now open /dev/mem and read string directly from mem.
	fd_devmem = open("/dev/mem", O_RDONLY);
	if (fd_devmem < 0) {
		perror("open(\"/dev/mem\") failed\n");
		goto exit;
	}

	result = lseek(fd_devmem, addr.phys, SEEK_SET);
	printf("lseek(%d, 0x%08lx) returned: 0x%08x\n", fd_devmem, addr.phys, result);
	if (result < 0) {
		perror("lseek() failed\n");
		goto exit;
	}

	// read the string and print it out
	memset(buf, 0, bufsize); 
	count = read(fd_devmem, buf, bufsize-1);
	printf("%s\n", buf);

exit:
	if (0 < fd_devmem) close(fd_devmem);
	if (0 < fd_bksbuf) close(fd_bksbuf);
	if (buf) free(buf);

	return rc;
}


