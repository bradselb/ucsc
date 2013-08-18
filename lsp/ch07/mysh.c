#define MYSH_PROMPT "mysh> "
#define BUFFER_SIZE 4096

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>

// --------------------------------------------------------------------------
// these fctns are defined in other source files in this directory.
int cat(int argc, char** argv);
int wc(int argc, char** argv);
int ls(int argc, char** argv);

// --------------------------------------------------------------------------
int do_interactive_loop(int read_fd, int write_fd);
int do_cmd(char* buf, int len);
int parse_cmd(char* buf, char** vbuf, int argcount);
int do_built_in_cmd(int argc, char** argv);
int do_external_cmd(char** argv);
int print_wait_status(FILE* wfp, int pid, int st);

void sigchild_handler(int);

// --------------------------------------------------------------------------
sig_atomic_t g_terminate; // parent and child see differnt variables.

// --------------------------------------------------------------------------
// intent of this was that when the child (server) process exits, the parent
// whould deal with it cleanly. Problem is that every external command forks
// and exec's a child too and each of these causes sigchild to be raised when
// it terminates. So, this doesn't really work as intended. Luckily, it does
// not appear to be necessary.
void sigchild_handler(int nr)
{
    if (nr == SIGCHLD) {
        //g_terminate = 1;
    }
}



// --------------------------------------------------------------------------
int do_interactive_loop(int rdfd, int wrfd)
{
    char* prompt;
    char* buf;
    int bufsize;
    int read_byte_count;
    int timeout;
    struct pollfd ps;
    int poll_rc;

    timeout = 15000; // fifteen seconds.

    // allcocate an input buffer.
    bufsize = BUFFER_SIZE ;
    buf = malloc(bufsize);
    if (!buf) {
        goto out;
    }


    if (isatty(rdfd)) {
        prompt = MYSH_PROMPT;
    } else {
        // if not a terminal...do not show a prompt.
        prompt = "";
    }

    // set up our poll struct.
    ps.fd = rdfd;
    ps.events = POLLIN;
    ps.revents = 0;


    while (!g_terminate) {
        if (prompt) {
            fprintf(stderr, "%s", prompt);
        }


        // wait a while for input to be ready.
        poll_rc = poll(&ps, 1, timeout);

        if (0 == poll_rc) {
            // timed out... ???
            //fprintf(stderr, "...timed out. Exiting.\n");
            break;
        } else if (poll_rc < 0) {
            // an error.
            perror("poll()");
            break;
        } // else
        // input fd is ready to be read.


        memset(buf, 0, sizeof buf);
        read_byte_count = read(rdfd, buf, bufsize-1);

        if ((0 < read_byte_count) && *buf) {
            // instead of doing the command here. just pass the line to the server.
            // do_cmd(buf, bufsize);
            write(wrfd, buf, read_byte_count);
        }

        // need to evaluate whether the child exited and if it did, then
        // we also need to exit.

    } // while

out:
    if (buf) {
        free(buf);
    }

    return 0;
}


// --------------------------------------------------------------------------
int do_non_interactive_loop(int rdfd)
{
    char* buf;
    int bufsize;
    int read_byte_count;
    int timeout;

    timeout = 500; // half second.

    // allcocate an input buffer.
    bufsize = BUFFER_SIZE ;
    buf = malloc(bufsize);
    if (!buf) {
        goto out;
    }
    memset(buf, 0, sizeof buf);

    while (!g_terminate) {

        read_byte_count = read(rdfd, buf, bufsize-1);
//fprintf(stderr, "(%s:%d) %s(), read %d bytes, '%s'\n", __FILE__, __LINE__, __FUNCTION__, read_byte_count, buf);


        if (0 == read_byte_count) {
            // we're done here.
            break;
        }

        if (read_byte_count < 0) {
            // an error...
//fprintf(stderr, "(%s:%d) %s(), read() returned: %d\n", __FILE__, __LINE__, __FUNCTION__, read_byte_count);
            break;
        }

        if (0 == *buf) {
            // empty line?
            continue;
        }

        int ec = do_cmd(buf, bufsize);
        if (ec) {
            // cannot really get here as it is right now
            // because, do_external_cmd always returns zero.
            // really want an indication that the internal command was 'exit'
//fprintf(stderr, "(%s:%d) %s(), do_cmd() returned: %d\n", __FILE__, __LINE__,__FUNCTION__, ec);

            break;
        }

        memset(buf, 0, sizeof buf);
    } // while

out:
    if (buf) {
        free(buf);
    }

    return 0;
}



// --------------------------------------------------------------------------
int do_cmd(char* buf, int bufsize)
{
    int rc = 0;
    char* arg_list[32];
    int arg_count;
    int max_arg_count = sizeof arg_list / sizeof arg_list[0];

    memset(arg_list, 0, sizeof arg_list);

    arg_count = parse_cmd(buf, arg_list, max_arg_count - 1);
    printf("(%s:%d) %s(),  arg_count: %d\n", __FILE__, __LINE__, __FUNCTION__, arg_count);

    if (arg_count < 1) {
        ; // nothing to do.
    } else if (0 == (rc=do_built_in_cmd(arg_count, arg_list))) {
        ; // successfully handled an internal command
    } else if (0 == (rc=do_external_cmd(arg_list))) {
        ; // successfully handled external command
    }

    // free the arg list
    for (int i=0; i<arg_count; ++i) {
        if (arg_list[i]) {
            free(arg_list[i]);
        }
    }
    return rc;
}


// --------------------------------------------------------------------------
int parse_cmd(char* buf, char** args, int n)
{
    int i = 0;
    const char* delim = " ,\t\n";
    char* token;
    size_t len;

    token = strtok(buf, delim);
    while (token && i < n) {
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

    return i; // this is arg_count.
}



// --------------------------------------------------------------------------
// returns: non-zero if this argv is a built-in command.
int do_built_in_cmd(int argc, char** argv)
{
    int rc;
    int ec;

    rc = 0;
    if ((0 == strncmp(argv[0], "exit", 4)) || (0 == strncmp(argv[0], "quit", 4))) {
        rc = 0; // yes, this was a built-in.
        g_terminate = 1;
    } else if (0 == strncmp(argv[0], "cd", 2)) {
        // change directory.
        if (argv[1] && (0 != (ec = chdir(argv[1])))) {
            fprintf(stderr, "failed to change directory to '%s'\n", argv[1]);
            rc = -1; // this internal command failed.
        }
    } else if (0 == strncmp(argv[0], "ls", 2)) {
        // list the directory
        rc = ls(argc, argv);
    } else if (0 == strncmp(argv[0], "cat", 3)) {
        // cat the files named on the cmd line
        rc = cat(argc, argv);
    } else if (0 == strncmp(argv[0], "wc", 2)) {
        // count the chars, words and lines
        rc = wc(argc, argv);
    } else if (0 == strncmp(argv[0], "hello", 5)) {
        // give a friendly greeting.
        fprintf(stderr, "\nHello! from process %d\n", getpid());
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
        exit(-1); // hmmm... maybe not?
    } else if (0 == pid) {
        // child process
        int ec;
        ec = execvp(argv[0], argv);
        if (ec < 0) {
            fprintf(stderr, "execvp(%s) returned: %d\n", argv[0], ec);
            _exit(errno);  // terminate the child
        }
    } else {
        // parent process
        int st;
        waitpid(pid, &st, 0);
        // print_wait_status(stdout, pid, st);
    }
    return 0;
}

// --------------------------------------------------------------------------
int print_wait_status(FILE* wfp, int pid, int st)
{
    fprintf(wfp, "\n");

    fprintf(wfp,"waitpid() returned: %6d",pid);

    if (WIFEXITED(st)) {
        fprintf(wfp,"exit: %3d\n", WEXITSTATUS(st));
    } else if (WIFSTOPPED(st)) {
        fprintf(wfp,"stop status: %3d\n", WSTOPSIG(st));
    } else if (WIFSIGNALED(st)) {
        fprintf(wfp,"termination signal: %3d\n", WTERMSIG(st));

        // fprintf(wfp,"\tcore dump: %s\n", WIFCORE(st) ? "yes" : "no");
    }

    return 0;
}

// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
    int pipefd[2]; // two ends of the pipe.
    pid_t pid;
    int ec;

    ec = pipe(pipefd);
    if (ec != 0) {
        fprintf(stderr, "(%s:%d) %s(), pipe() returned: %d [%s]\n", __FILE__,__LINE__, __FUNCTION__, ec, strerror(errno));
        // perror("pipe");
        goto OUT;
    }

    // handle the SIGCHLD signal.
    signal(SIGCHLD, sigchild_handler);

    pid = fork();
    if (0 == pid) {
        // ---------------------- child -----------------------
        // child is the "server".
        // communication is from parent to child
        // the child does not talk back to parent (until he gets older).

        close(pipefd[1]); // close the write end of the pipe.
        do_non_interactive_loop(pipefd[0]); // read from pipe, and do what the parent says!
        close(pipefd[0]); // we're done.  close the read end.
        _exit(0);



    } else if (0 < pid) {
        // ---------------------- parent -----------------------

        close(pipefd[0]);// close read end of pipe

        do_interactive_loop(0, pipefd[1]); // read from stdin, write to pipe.

        // if we drop out of the interactive loop for any reason, close the write end
        // of the pipe. This will tell the child that is is time to terminate.
        close(pipefd[1]);

        int st;
        waitpid(pid, &st, 0);

    } else { // pid < 0
        // an error.
        fprintf(stderr, "(%s:%d) %s(), fork() returned: %d [%s]\n", __FILE__, __LINE__, __FUNCTION__ , pid, strerror(errno));
    }

OUT:
    return 0;
}
