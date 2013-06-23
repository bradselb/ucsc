#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// a program that copies the contents of files to stdout. 
// a simplified version of the cat utility.
// uses fgets() and kinda assumes that the file is plain 
// text. 


int main(int argc, char** argv)
{
    FILE* file;
    int show_line_nr = 0;
    int line_nr;
    char* buf = 0;
    int bufsize = 256;

    int i, j;
    char* arg; // make convenient notation.

    buf = malloc(bufsize);

    for (i=1; i<argc; ++i) {
        arg = argv[i];
        
        if ('-' == arg[0]) {
            // this is a cmd line option

            // iterate over individual chars to accum options
            for (j=1; j<strlen(arg); ++j) {
                 switch (arg[j]) {
                    case 'l':
                        show_line_nr = 1;
                        break;
                    default:
                        break;
                 } // switch
            } // for each option letter

        } else {
            // assume that arg is a filename.

            // open the file and copy it line by line to stdout.
            if (0 != (file = fopen(arg, "r"))) {
                line_nr = 0;
                while (fgets(buf, bufsize, file)) {
                    ++line_nr;
                    if (show_line_nr) {
                        printf("%8d  %s", line_nr, buf);
                    } else {
                        printf("%s", buf);
                    }
                }
                close(file);
            }
        } // else its a filename 

    } // for each arg in the cmd line arg list

    if (buf) free(buf);
    return 0;
}
