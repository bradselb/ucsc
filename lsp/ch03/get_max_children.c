#include <stdio.h>
#include <unistd.h>
#include <errno.h>

// linux systems programming, summer 2013
// Brad Selbrede

// a short program that uses the sysconf() fucntion to determine
// the maximum allowed number of simultaneous child processes.
//
// assignment #3, question #4
// 



int main(int argc, char** argv)
{
    long max = sysconf(_SC_CHILD_MAX);

    errno = 0; // explicitly set it so that we can check it after the call to sysconf()
    if (!errno && max < 0) {
        printf("apparenltly, the max allowed number of child processes is undefined\n");
    } else if (max < 0) {
        perror("sysconf(): ");
    } else {
        printf("sysconf() reports max allowed number of child processes is: %ld\n", max);
        fflush(stdout);
    }

    return 0;
}

