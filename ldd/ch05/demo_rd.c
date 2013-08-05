#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memset()
#include <errno.h> // perror()
//#include <sys/ioctl.h>


int main(int argc, char **argv)
{
	int rc = 0;

	// allocate a buffer
	size_t bufsize = 512; // do not exceed 4096
	char* buf = malloc(bufsize);
	if (!buf) {
		printf("failed to allocate buffer of size, %lu\n", bufsize);
		goto exit;
	}

	// open the device file
	FILE* file;
	const char filename[] = "/dev/bks";
	file = fopen(filename, "r");
	if (!file) {
		printf("failed to open file named '%s'\n", filename);
		goto exit;
	}


	// read the string a chunk at a time and print it out
	ssize_t count = 0;
	int chunk_size = 16; // must NOT be greater than bufsize.
	memset(buf, 0, bufsize);
	while (0 < (count = fread(buf, 1, chunk_size, file))) {
		printf("%s", buf);
		fflush(stdout);
		memset(buf, 0, chunk_size+1);
	}
	fflush(stdout);


exit:
	if (file) fclose(file);
	if (buf) free(buf);

	return rc;
}


