#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> // O_RDONLY flag


// Copyright (C) 2013 Bradley K. Selbrede - all rights reserved.


// a simplified version of the cat utility.
// a function that opens files and copies their contents to the given fd.


int cat(int argc, char** argv, int wfd)
{
    int rc = 0;
    int rfd;
    char* buf = 0;
    int bufsize = 4096;
    ssize_t count;

    char* arg; // make convenient notation.

    buf = malloc(bufsize);
    if (!buf) {
        rc = -ENOMEM;
        buf = "failed to allocate buffer.\n";
        count = strlen(buf);
        write(wfd, buf, count);
        goto out;
    }

    for (int i=1; i<argc; ++i) {
        arg = argv[i];

        rfd = open(arg, O_RDONLY);
        if (rfd < 0) {
            ssize_t len = 0;
            memset(buf, 0, bufsize);
            len = sprintf(buf, "could not open file named '%s'\n", arg);
            strerror_r(rfd, buf+len, bufsize-len);
            len = strnlen(buf, bufsize);
            write(wfd, buf, len);
        } else {
            while ((count = read(rfd, buf, bufsize)) > 0) {
                write(wfd, buf, count);
                memset(buf, 0, bufsize); // un-necessary?
            }
            close(rfd);
        }
    } // for each arg in the cmd line arg list

out:
    if (buf) {
        free(buf);
    }
    return rc;
}
