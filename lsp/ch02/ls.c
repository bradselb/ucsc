#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

// Copyright (C) 2013 Bradley K. Selbrede - all rights reserved.
// 

// ls.c 
// a short program that is intended to approximate the behaviour of the ls 
// system utility.
//
// usage: ls [<options>]  [<path> | <filename>]
// list the content of the directory named by path cmd line argument
// or, if a file name is given as a command line arg, simply list the filename.
//
// options: 
//    -l : long listing (show the details for each file in dir). 


static void list_directory(const char* name, int show_details);
static void list_file(const char* name, int show_details);
static char* encode_permissions(mode_t m);


int main(int argc, char* argv[])
{

    int details; // holds state of '-l' cmd line option
    int i; // an index into the argv array
    int i0; // which cmd line arg to start with
    char* arg; // convenience.

    i0 = 1; // initially, we start looking for cmd line args with argv[1]
    details = 0;
    arg = argv[1]; // note that if argc == 0 then arg == 0
    if (arg && '-' == arg[0] && 'l' == arg[1]) {
        details = 1;
        i0 = 2; // keep track of the fact that we consumed one cmd line arg.
    } 

printf("argc: %d, i0: %d, details: %d, arg: %s\n", argc, i0, details, argv[i0]);

    if (1 == argc || (2 == argc && details)) {
        // no path and no filename on cmd line so, 
        // just list content of current working dir.
        list_directory("./", details);
    } else {
        // a path or filename was specified on the cmd line

        // iterate over the remaining cmd line args.
        // assuming that each arg is either a path or filename
        // figure out which it is and the list the dir or file 
        for (i=i0; i<argc; ++i) {
            arg = argv[i];
            list_directory(arg, details);
        }

    } // else a specific path was give on cmd line.

    return 0;
}


// a helper function
static void list_directory(const char* name, int show_details)
{
    struct stat st;
    DIR* dir;
    struct dirent* dirent;
    int rc;

    rc = stat(name, &st);
    if (rc < 0) {
        // bogus filename or path.
        fprintf(stderr, "cannot access '%s'. No such file or directory.\n");
    } else if (S_ISDIR(st.st_mode)) {
        if (0 != (dir = opendir(name))) {
            // success!
            while (0 != (dirent = readdir(dir))) {
                list_file(dirent->d_name, show_details);
            }
            closedir(dir);
        } else {
            // could not open that dir for some reason.
        }
    } else if (S_ISREG(st.st_mode)) {
        // name is the name of a regular file.
        list_file(name, show_details);
    }
}

// and another...
static void list_file(const char* name, int show_details)
{
    struct stat st;
    int rc;

    if (name && *name != '.') {
        rc = stat(name, &st);
        printf("(%s:%d) %s(),  stat(%s) returned: %d\n", __FILE__, __LINE__, __FUNCTION__, name, rc);
        if (show_details && !rc) {
            printf("%s %ld %ld %s\n",encode_permissions(st.st_mode), st.st_uid, st.st_gid, name);
        } else {
            printf("%s\n", name);
        }
    }
}


char* encode_permissions(mode_t m)
{
    static char buf[12];
    memset(buf, 0, sizeof(buf));
    buf[0] = (S_ISDIR(m) ? 'd' : '-');

    buf[1] = (m & S_IRUSR ? 'r' : '-');
    buf[2] = (m & S_IWUSR ? 'w' : '-');
    buf[3] = (m & S_IXUSR ? 'x' : '-');

    buf[4] = (m & S_IRGRP ? 'r' : '-');
    buf[5] = (m & S_IWGRP ? 'w' : '-');
    buf[6] = (m & S_IXGRP ? 'x' : '-');

    buf[7] = (m & S_IROTH ? 'r' : '-');
    buf[8] = (m & S_IWOTH ? 'w' : '-');
    buf[9] = (m & S_IXOTH ? 'x' : '-');
    return buf;
}
