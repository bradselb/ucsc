#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memset()
#include <errno.h> // perror()
#include <unistd.h> //read(), write()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

static const char* G_DeviceName = "/dev/bks";

int main(int argc, char **argv)
{
	int rc = 0;
	int fd = 0;
	unsigned int buf[16];
	size_t count = 0;
	void* pMapped = 0;

	memset(buf, 0, sizeof buf); 

	fd = open("/dev/bks", O_RDWR);
	pMapped = mmap(0, sizeof buf, PROT_READ, MAP_PRIVATE, fd, 0);
	munmap(pMapped, sizeof buf);
	close(fd);

exit:

	return rc;
}


