#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>


#include "mysh_common.h"

#define MAX_ARGC 1024




// --------------------------------------------------------------------------
int do_cmd(char* buf, int bufsize)
{
    int rc = 0;
    int argc;
    char* argv[MAX_ARGC];


    if ((buf && *buf == 0)) {
        rc = 0;
        goto done;
    }


    memset(argv, 0, sizeof argv);

    argc = parse_cmd(buf, argv);

    if (argc < 1) {
        rc = 0;
        goto done;
    }
    
    rc = do_built_in_cmd(argc, argv);
    if (1 == rc) {
        rc = do_external_cmd(argv);
    }

    // free the arg list
    for (int i=0; i<argc; ++i) {
        if (argv[i]) {
            free(argv[i]);
        }
    }

done:
    return rc;
}


// --------------------------------------------------------------------------
int parse_cmd(char* buf, char** args)
{
    char* token;
    static const char* delim = " ,\t\n";
    int i = 0;
    int max_argc = MAX_ARGC;

    token = strtok(buf, delim);
    while (token && i < max_argc) {
        size_t len;

        len = strlen(token);
        args[i] = malloc(len + 1);
        if (!args[i]) {
            break;    // unlikely.
        }
        strcpy(args[i], token); // strcpy() copies the terminating zero.
        token = strtok(NULL, delim);
        ++i;
    }

    args[i] = 0; // it should already be the case.

    return i; // this is arg count.
}



// --------------------------------------------------------------------------
// returns: non-zero if this argv is a built-in command.
int do_built_in_cmd(int argc, char** argv)
{
    int rc;
    int ec;

    rc = 0;
    if ((0 == strncmp(argv[0], "exit", 4)) || (0 == strncmp(argv[0], "quit", 4))) {
        //fprintf(stderr, "good-bye (pid: %d).\n", getpid());
        kill(getppid(), SIGUSR1);

    } else if (0 == strncmp(argv[0], "cd", 2)) {
        if (argv[1] && (0 != (ec = chdir(argv[1])))) {
            fprintf(stderr, "failed to change directory to '%s'\n", argv[1]);
        }
    } else if (0 == strncmp(argv[0], "ls", 2)) {
        ls(argc, argv);

    } else if (0 == strncmp(argv[0], "cat", 3)) {
        cat(argc, argv);

    } else if (0 == strncmp(argv[0], "wc", 2)) {
        wc(argc, argv);

    } else if (0 == strncmp(argv[0], "hello", 5)) {
        fprintf(stderr, "Hello! (pid: %d)\n", getpid());

    } else if (0 == strncmp(argv[0], "help", 4)) {
        fprintf(stderr, "sorry. no help available\n");

    } else {
        // this is not an internal command.
        rc = 1;
    }

    return rc;
}


// --------------------------------------------------------------------------
int do_external_cmd(char** argv)
{
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "(%s:%d) %s(), fork() returned: %d\n", __FILE__, __LINE__, __FUNCTION__ , pid);

    } else if (0 == pid) {
        // child process does the external command
        int ec;
        ec = execvp(argv[0], argv);
        if (ec < 0) {
            fprintf(stderr, "execvp(%s) returned: %d\n", argv[0], ec);
            _exit(errno);  // terminate the child
        }

    } else {
        // parent process just waits for child to complete.
        int st;
        waitpid(pid, &st, 0);
    }

    return 0;
}


