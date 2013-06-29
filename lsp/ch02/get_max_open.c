#include <stdio.h>
#include <unistd.h>
//#include "flist.h" /* a container of file handles */

/* a short program to determine...
 *   the maximum allowed number of open files using the sysconf() function.
 *
 * this is part of assignment #2 for the linux systems programming class.
 * 
 * Brad Selbrede
 */



int main(int argc, char** argv)
{
    long max_open_files = sysconf(_SC_OPEN_MAX);

    if (max_open_files < 0) {
        perror("sysconf(_SC_OPEN_MAX): ");
    } else {
        printf("sysconf() reports max open files is: %ld\n", max_open_files);
        fflush(stdout);
    }

    return 0;
}

