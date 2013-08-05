#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h> // need printf()
//#include <string.h>


int main(int argc, char** argv)
{
	int fd = 0;
	off_t offset;

	fd = open("/dev/bks", O_RDWR);
	if (fd < 0) {
		printf("open() failed.\n");
		goto exit;
	}

	offset = lseek(fd, 0, SEEK_SET);
	printf("lseek(fd, 0, SEEK_SET) --> offset: %d (expect: 0)\n", (int)offset);
	if (offset < 0) perror("unexpected");

	offset = lseek(fd, 1024, SEEK_SET);
	printf("lseek(fd, 1024, SEEK_SET) --> offset: %d (expect: 1024)\n", (int)offset);
	if (offset < 0) perror("unexpected");

	offset = lseek(fd, -256, SEEK_CUR);
	printf("lseek(fd, -256, SEEK_CUR) --> offset: %d (expect: 768)\n", (int)offset);
	if (offset < 0) perror("unexpected");

	offset = lseek(fd, -256, SEEK_CUR);
	printf("lseek(fd, -256, SEEK_CUR) --> offset: %d (expect: 512)\n", (int)offset);
	if (offset < 0) perror("unexpected");

	offset = lseek(fd, -2048, SEEK_END);
	printf("lseek(fd, -2048, SEEK_END) --> offset: %d (expect: 2047)\n", (int)offset);
	if (offset < 0) perror("unexpected");

	offset = lseek(fd, 0, SEEK_END);
	printf("lseek(fd, 0, SEEK_END) --> offset: %d (expect: 4095)\n", (int)offset);
	if (offset < 0) perror("unexpected");

	offset = lseek(fd, 2048, SEEK_END);
	printf("lseek(fd, 2048, SEEK_END) --> offset: %d (expect: -1)\n", (int)offset);
	if (offset < 0) perror("expected");

	offset = lseek(fd, 0, SEEK_SET);
	printf("lseek(fd, 0, SEEK_SET) --> offset: %d (expect: 0)\n", (int)offset);
	if (offset < 0) perror("unexpected");

	offset = lseek(fd, -256, SEEK_CUR);
	printf("lseek(fd, -256, SEEK_CUR) --> offset: %d (expect: -1)\n", (int)offset);
	if (offset < 0) perror("expected");

	close(fd);

exit:	
	return 0;
}
