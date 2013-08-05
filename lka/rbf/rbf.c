#define _GNU_SOURCE
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "rbf.h"
#include "rbf_help.h"
#include "slist.h"


// --------------------------------------------------------------------------
int rbfclose(void* ctx);
ssize_t rbfread(void* ctx, char* buf, size_t bufsize);
ssize_t rbfwrite(void* ctx, const char* buf, size_t bufsize);
int rbfseek(void* ctx, off64_t* position, int whence);

// --------------------------------------------------------------------------
static cookie_io_functions_t iofctns = {
	.read = rbfread,
	.write = rbfwrite,
	.seek = rbfseek,
	.close = rbfclose
};

// --------------------------------------------------------------------------
typedef struct RBF
{
	char* filename;
	struct slist* head;
	int count; // the current number of strings in list
	int max_count;
	
}rbf_t;

// --------------------------------------------------------------------------
FILE* rbfopen(const char* path, int size)
{
	FILE* file = 0;  // what this fctn returns.
	struct RBF* rbf = 0; // the context
	char* buf = 0;
	const int bufsize = 4096;

	//printf("(%s:%d) %s() path: '%s'\n", __FILE__, __LINE__, __FUNCTION__, path);

	rbf = malloc(sizeof(struct RBF));
	if (!rbf) {
		// not much can be done. let caller have a zero.
		goto EXIT;
	}

	rbf->filename = 0;
	if (path) {
		rbf->filename = malloc(1 + strlen(path));
		if (rbf->filename) {
			strcpy(rbf->filename, path);
		}
	}

	rbf->head = slist_alloc();
	if (!rbf->head){
		free(rbf);
		rbf = 0;
		goto EXIT;
	}
	
	
	rbf->count = 0;
	rbf->max_count = size;

	// if the file already exists, use content to pre-populate list
	buf = malloc(bufsize);
	if (buf && fexists(rbf->filename)) {
		memset(buf, 0, bufsize);
		FILE* file = fopen(rbf->filename, "r+");
		while (fgets(buf, bufsize, file)) {
			slist_push_back(rbf->head, buf);
			++rbf->count;
			if (rbf->max_count < rbf->count) {
				slist_pop_front(rbf->head);
				--rbf->count;
			}
		} // while
		fclose(file);
	}

	// now have a completely constructed RBF.
	file = fopencookie(rbf, "w", iofctns );
	if (!file) {
		rbfclose(rbf);
	}

EXIT:
	if (buf) free(buf);

	return file;
}

// --------------------------------------------------------------------------
int rbfclose(void* ctx)
{
	int rc = 0;
	struct RBF* rbf = (struct RBF*)ctx;

	//printf("(%s:%d) %s()\n", __FILE__, __LINE__, __FUNCTION__);

	if (rbf) {
		// flush the string list to the file? 
		if (rbf->filename) {
			free(rbf->filename);
			rbf->filename = 0;
		}

		if (rbf->head) {
			slist_free(rbf->head);
			rbf->head = 0;
		}

		rbf->count = 0;
	}
	return rc;
}

// --------------------------------------------------------------------------
// transfer upto bufsize bytes from the buffer.
// return: number of bytes transferred or,
//         -1 to indicate an error.
ssize_t rbfwrite(void* ctx, const char* buf, size_t bufsize)
{
	ssize_t bytes_transferred = -1;
	struct RBF* rbf = (struct RBF*)ctx;
	size_t length = strnlen(buf, bufsize);

	//printf("(%s:%d) %s(), %s, length : %d, bufsize: %u\n", __FILE__, __LINE__, __FUNCTION__, buf, length, bufsize);

	if (rbf && buf) {
		bytes_transferred = length;
		slist_push_front(rbf->head, buf);
		++rbf->count;
		if (rbf->max_count < rbf->count) {
			slist_pop_back(rbf->head);
			--rbf->count;
		}

		FILE* file = fopen(rbf->filename, "w");
		if (!file) {
			perror("rbfwrite");
		} else {
			struct slist* node = rbf->head;
			do {
				const char* s = slist_string(node);
				if (s) {fputs(s, file);}
				node = slist_next(node);
			} while (node != rbf->head);
			fclose(file);
		}
	}
	return bytes_transferred;
}


// --------------------------------------------------------------------------
// transfer upto bufsize bytes to the buffer.
// return: number of bytes transferred or,
//         zero to indicate end of file.
//         -1 to indicate an error.
ssize_t rbfread(void* ctx, char* buf, size_t bufsize)
{
	return 0;
}
// --------------------------------------------------------------------------
// 
int rbfseek(void* ctx, off64_t* offset, int whence)
{
	return 0;
}
