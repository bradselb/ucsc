#include <stdio.h>
#include <unistd.h>
//#include "flist.h" /* a container of file handles */

/* a short program to determine...
 *   1. the maximum number of open files allowed, both experimentally and 
 *      using the sysconf() function.
 *   2. the number of clock ticks per second using sysconf()
 *
 * this is part of assignment #1 for the linux systems programming class.
 * 
 * Brad Selbrede
 */



int main(int argc, char** argv)
{
    FILE* f;
    struct flist* flist = 0;

    long max_open_files = sysconf(_SC_OPEN_MAX);
    long ticks_per_sec = sysconf(_SC_CLK_TCK);

    if (max_open_files < 0) {
        perror("sysconf(_SC_OPEN_MAX): ");
    } else {
        printf("sysconf() reports max open files is: %ld\n", max_open_files);
    }

    if (ticks_per_sec < 0) {
        perror("sysconf(_SC_CLK_TCK): ");
    } else {
        printf("sysconf() reports clock ticks per second is: %ld\n", ticks_per_sec);
    }

    /* determine max open files empircally... */
    max_open_files = 0; 
    while (0 != (f=tmpfile())) {
        ++max_open_files;
//        push_back(flist, f);
    }

    printf("empiracally determined max open files is: %ld\n", max_open_files);
    printf("plus stdin, stdout and stderr makes, %ld\n", max_open_files+3);
    fflush(stdout);

    /* files are automagically closed and deleted when program terminates. */
//    while (0 != (f = pop_front(flist))) {
//        fclose(f);
//    }

    return 0;
}


