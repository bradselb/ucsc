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
int do_interactive_loop(int write_fd, int read_fd);
int do_non_interactive_loop(int read_fd, int write_fd);
int do_cmd(char* buf, int len);
int parse_cmd(char* buf, char** vbuf, int argcount);
int do_built_in_cmd(int argc, char** argv);
int do_external_cmd(char** argv);
int print_wait_status(FILE* wfp, int pid, int st);

void signal_handler(int);

// --------------------------------------------------------------------------
sig_atomic_t g_terminate; // parent and child see different variables.

// --------------------------------------------------------------------------
// intent:
// when the user types 'exit' or 'quit' at the prompt, we want both processes
// to exit cleanly and quickly. In this case, the child (server) process
// recognizes the exit command and sets the g_terminate global varible in 
// it's own process address space. That makes it drop out of the 
// non-interactive-loop. The last thing it does as it is exiting is send 
// a signal to the parent process indicating that it is time to exit.
void signal_handler(int nr)
{
    if (nr == SIGUSR1) {
        g_terminate = 1;
    }
}



// --------------------------------------------------------------------------
int do_interactive_loop(int wrfd, int rdfd)
{
    char* prompt = 0;
    char* buf;
    int bufsize;
    int read_byte_count;
    int timeout;
    struct pollfd ps;
    int poll_rc;

    // how long will we wait for a response from the child? 
    timeout = 15000; // milliseconds.

    // initialize a poll struct so that we can wait for child to respond
    ps.fd = rdfd;
    ps.events = POLLIN;
    ps.revents = 0;


    // allcocate an input buffer.
    bufsize = BUFFER_SIZE ;
    buf = malloc(bufsize);
    if (!buf) {
        goto out;
    }


    if (isatty(0)) {
        prompt = MYSH_PROMPT;
    }


    while (!g_terminate) {
        if (prompt) {
            fprintf(stderr, "%s", prompt);
        }

        // read a line of input from the user
        memset(buf, 0, bufsize);
        read_byte_count = read(0, buf, bufsize-1);

        if ((0 < read_byte_count) && *buf) {
            // tell the child
            write(wrfd, buf, read_byte_count);
        }

        // wait a while for child to reply
        poll_rc = poll(&ps, 1, timeout);

        if (0 == poll_rc) {
            // timed out.
            break;
        } else if (poll_rc < 0) {
            // an error.
            fprintf(stderr, "(%s:%d) %s(), pipe() returned: %d [%s]\n", __FILE__, __LINE__, __FUNCTION__ , poll_rc, strerror(errno));
            break;
        } // else...
        // child replied. get the response from child.

        memset(buf, 0, bufsize);
        read_byte_count = read(rdfd, buf, bufsize-1);
        //fprintf(stderr, "%s\n", buf);

        // this parent isn't really paying very close attention to his child. 
        // it doesn't really matter what the child says - the parent only
        // does something if the child says it is finished. 

        //if (0 == strncmp(buf, "ok", 2)) {
        //    // it's all good. 
        //    continue;
        //} 

        if (0 == strncmp(buf, "done", 4)) {
            break;
        }

    } // while

out:
    if (buf) {
        free(buf);
    }

    //fprintf(stderr, "(%s:%d) %s() - bye.\n", __FILE__, __LINE__,__FUNCTION__);

    return 0;
}


// --------------------------------------------------------------------------
int do_non_interactive_loop(int rdfd, int wrfd)
{
    char* buf;
    int bufsize;
    int read_byte_count;
    int timeout;
    char reply[] = {"ok"};

    timeout = 500; // half second.

    // allcocate an input buffer.
    bufsize = BUFFER_SIZE ;
    buf = malloc(bufsize);
    if (!buf) {
        goto out;
    }


    while (!g_terminate) {

        memset(buf, 0, bufsize);
        read_byte_count = read(rdfd, buf, bufsize-1);

        if (0 == read_byte_count) {
            // we're done here.
            break;
        }

        if (read_byte_count < 0) {
            // an error...
            break;
        }

        if (0 == *buf) {
            // empty line?
            continue;
        }

        do_cmd(buf, bufsize);

        if (g_terminate) {
            write(wrfd, "done", 5);
        } else {
            write(wrfd, reply, sizeof reply);
        }

    } // while

out:
    if (buf) {
        free(buf);
    }

    kill(getppid(), SIGUSR1);

    //fprintf(stderr, "(%s:%d) %s() - bye.\n", __FILE__, __LINE__,__FUNCTION__);

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
    //printf("(%s:%d) %s(),  arg_count: %d\n", __FILE__, __LINE__, __FUNCTION__, arg_count);

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
    int parent_to_child[2]; // two ends of the pipe.
    int child_to_parent[2];
    pid_t pid;
    int ec;

    ec = pipe(parent_to_child);
    if (ec != 0) {
        fprintf(stderr, "(%s:%d) %s(), pipe() returned: %d [%s]\n", __FILE__,__LINE__, __FUNCTION__, ec, strerror(errno));
        // perror("pipe");
        goto OUT;
    }

    ec = pipe(child_to_parent);
    if (ec != 0) {
        fprintf(stderr, "(%s:%d) %s(), pipe() returned: %d [%s]\n", __FILE__,__LINE__, __FUNCTION__, ec, strerror(errno));
        close(parent_to_child[0]);
        close(parent_to_child[1]);
        // perror("pipe");
        goto OUT;
    }


    // handle a user signal.
    signal(SIGUSR1, signal_handler);

    pid = fork();
    if (0 == pid) {
        // ---------------------- child -----------------------
        // child is the "server".
        // child receives commands from parent 
        // and only replies 'ok' or 'done'
        close(parent_to_child[1]); // close the write end of the pipe.
        close(child_to_parent[0]); // close read end. 

        do_non_interactive_loop(parent_to_child[0], child_to_parent[1]);

        close(parent_to_child[0]); // we're done.  close the read end.
        close(child_to_parent[1]); // close write end too.
        _exit(0);


    } else if (0 < pid) {
        // ---------------------- parent -----------------------

        close(parent_to_child[0]);// close read end of pipe
        close(child_to_parent[1]); // close write end. 

        do_interactive_loop(parent_to_child[1], child_to_parent[0]);

        // if we drop out of the interactive loop for any reason, close the write end
        // of the pipe. This will tell the child that is is time to terminate.
        close(parent_to_child[1]);
        close(child_to_parent[0]); // close read end. 

        //fprintf(stderr, "(%s:%d) %s(), parent waits for child to exit.\n", __FILE__, __LINE__, __FUNCTION__ );

        int st;
        waitpid(pid, &st, 0);

    } else { // pid < 0
        // an error.
        fprintf(stderr, "(%s:%d) %s(), fork() returned: %d [%s]\n", __FILE__, __LINE__, __FUNCTION__ , pid, strerror(errno));
    }

OUT:
    return 0;
}
