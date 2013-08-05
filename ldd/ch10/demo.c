#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memset()
#include <errno.h> // perror()
//#include <unistd.h> //read(), write()
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>
//#include <sys/ioctl.h>

static const char* G_DeviceName = "/dev/bks";

int main(int argc, char **argv)
{
	int rc = 0;
	FILE* file = 0;
	unsigned int buf[16];
	size_t count = 0;

	memset(buf, 0, sizeof buf); 

	// open the device file.
	file = fopen(G_DeviceName, "r");
	if (!file) {
		perror("open device file failed\n");
		goto exit;
	}


	int i;
	for (i=0; i<3; ++i) {
		printf("move or click the mouse\n");
		fflush(stdout);
		sleep(5-i);

		size_t elem_size = sizeof buf[0];
		size_t elem_count = sizeof buf / sizeof buf[0];
		while (0 < (count = fread(buf, elem_size, elem_count, file))) {
			int i;
			for (i=0; i<count; ++i) {
				printf("%02x ", buf[i]);
			}
			printf("\n");
			fflush(stdout);
			memset(buf, 0, sizeof buf); 
		}
		fflush(stdout);
	}

exit:
	if (file) fclose(file);

	return rc;
}


