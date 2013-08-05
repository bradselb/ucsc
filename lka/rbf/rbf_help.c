#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int fexists(const char* path)
{
	int exists = 0;
	FILE* file;

	if (path && (file = fopen(path, "r"))) {
		fclose(file);
		exists = 1;
	}
	return exists;
}

int fsize(const char* path, size_t* size)
{
	int ec = 0;
	struct stat s;

	if (!(path && size)) {
		ec = -1;
	} else if (0 == (ec=stat(path, &s))) {
		*size = s.st_size;
	}
	return ec;
}

