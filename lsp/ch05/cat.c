
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Copyright (C) 2013 Bradley K. Selbrede - all rights reserved.
// 


// a function that copies the contents of files to stdout. 
// a simplified version of the cat utility.
// uses fgets() and kinda assumes that the file is plain 
// text. 


int cat(int argc, char** argv)
{
    int rc = 0;
    FILE* file;
    int show_line_nr = 0;
    int line_nr;
    char* buf = 0;
    int bufsize = 256;

    int i, j;
    char* arg; // make convenient notation.

    buf = malloc(bufsize);
    if (!buf) {
        rc = -ENOMEM;
        goto out;
    }

    for (i=1; i<argc; ++i) {
        arg = argv[i];
        
        if ('-' == arg[0]) { // this is a cmd line option
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

        } else { // assume that arg is a filename.
            // open the file and copy it line by line to stdout.
            file = fopen(arg, "r");
            if (!file) {
                fprintf(stderr, "could not open file named '%s'.\n", arg);
            } else {
                line_nr = 0;
                while (fgets(buf, bufsize, file)) {
                    ++line_nr;
                    if (show_line_nr) {
                        printf("%8d  %s", line_nr, buf);
                    } else {
                        printf("%s", buf);
                    }
                } // while not EOF.
                fclose(file);
            }
        } // else its a filename 
    } // for each arg in the cmd line arg list

out:
    if (buf) free(buf);
    return rc;
}
