#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

char* infilename = "redirect_in.txt";
char* outfilename = "redirect_out.txt";

int main(int argc, char* argv[])
{
    int infd = -1;
    int outfd = -1;
    int count = 0;
    int rc;
    int c;

    // open the file that will bre used as stadin
    infd = open(infilename, O_RDONLY);
    if (infd < 0) {
        perror("open input file");
        goto out;
    }

    // redirect stdin
    close(0);
    rc = dup2(infd, 0);
    if (rc < 0) {
        perror("dup2 input file");
        goto out;
    }

    // oen the file that will be used in place of stdout
    outfd = open(outfilename, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if (outfd < 0) {
        perror("open output file");
        goto out;
    }

    // redirect stdout 
    close(1);
    rc = dup2(outfd, 1);
    if (rc < 0) {
        perror("dup2 output file");
        goto out;
    }

    // copy from stdin to stdout
    while ( (c = getchar()) != EOF ) {
        putchar(c);
        ++count;
    }

    fprintf(stderr, "transferred %d bytes from file '%s' to file '%s'\n", count, infilename, outfilename);


out:
    if (infd > 0) close(infd);
    if (outfd > 0) close(outfd);

    return 0;
}
