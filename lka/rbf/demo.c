#include <stdio.h>
#include <time.h>

#include "rbf.h"

int main(int argc, char** argv)
{
	FILE* file;
	time_t t;
	int i = 0;
	file = rbfopen("demo.txt", 10);
	for (i = 0; i < 3; ++i)
	{
		time(&t);
		fprintf(file, "(%lu) this is line #%d\n", t, i);
		fflush(file);
		sleep(2);

	}
//	fflush(file);
	fclose(file);
	return 0;
}
