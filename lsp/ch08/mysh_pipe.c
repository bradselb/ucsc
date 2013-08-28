#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>


#include "mysh_pipe.h"
#include "mysh_common.h"

// this is a hack...fix it.
extern sig_atomic_t g_terminate;


// --------------------------------------------------------------------------
static int do_interactive_loop(int write_fd, int read_fd);
static int do_non_interactive_loop(int read_fd, int write_fd);


// --------------------------------------------------------------------------
int mysh_implemented_with_pipe(void)
{
    int ec = 0;
    int parent_to_child[2]; // two ends of the pipe.
    int child_to_parent[2];
    pid_t pid;

    ec = pipe(parent_to_child);
    if (ec != 0) {
        perror("parent_to_child pipe");
        goto out;
    }

    ec = pipe(child_to_parent);
    if (ec != 0) {
        close(parent_to_child[0]);
        close(parent_to_child[1]);
        perror("child-to-parent pipe");
        goto out;
    }


    pid = fork();
    if (0 == pid) {
        // ---------------------- child -----------------------
        // child is the "server".
        // child receives commands from parent 
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

out:
    return ec; 
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
    timeout = 10000; // milliseconds.

    // initialize a poll struct so that we can wait for child to respond
    ps.fd = rdfd;
    ps.events = POLLIN;
    ps.revents = 0;


    // allocate an input buffer.
    bufsize = MYSH_BUFFER_SIZE ;
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
        read_byte_count = read(STDIN_FILENO, buf, bufsize-1);

        if ((0 < read_byte_count) && *buf) {
            // tell the child
            write(wrfd, buf, read_byte_count);
        }

        // wait a while for child to reply
        poll_rc = poll(&ps, 1, timeout);

        if (0 == poll_rc) {
            // timed out.
            fprintf(stderr, "(%s:%d) %s(), poll() returned: %d [%s]\n", __FILE__, __LINE__, __FUNCTION__ , poll_rc, strerror(errno));
            break;
        } else if (poll_rc < 0) {
            // an error.
            fprintf(stderr, "(%s:%d) %s(), poll() returned: %d [%s]\n", __FILE__, __LINE__, __FUNCTION__ , poll_rc, strerror(errno));
            break;
        } else {
            // the child replied. 

            // get the response from child.
            memset(buf, 0, bufsize);
            read_byte_count = read(rdfd, buf, bufsize-1);
            //fprintf(stderr, "%s\n", buf);

            // If the child doesn't respond in the affirmative, then 
            // the parent bails out of the interactive loop. 
            if (0 != strncmp(buf, "aye", 3)) {
                break;
            }
        }

    } // while

out:
    if (buf) {
        free(buf);
    }

    //fprintf(stderr, "%s(), g_terminate: %d\n", __FUNCTION__, g_terminate);

    return 0;
}


// --------------------------------------------------------------------------
int do_non_interactive_loop(int rdfd, int wrfd)
{
    char* buf;
    int bufsize;
    int read_byte_count;
    static char ack[] = {"aye"};
    static char nack[] = {"nay"};


    // allcocate an input buffer.
    bufsize = MYSH_BUFFER_SIZE ;
    buf = malloc(bufsize);
    if (!buf) {
        goto out;
    }


    while (1) {
        int rc;

        memset(buf, 0, bufsize);
        read_byte_count = read(rdfd, buf, bufsize-1);

        if (read_byte_count <= 0) {
            // 0 ==> parent closed his (write) end of pipe.
            // negative ==> error.
            //fprintf(stderr, "(%s:%d) %s(), read() returned: %d [%s]\n", __FILE__, __LINE__, __FUNCTION__ , read_byte_count, strerror(errno));
            break;
        }

        rc = do_cmd(buf, bufsize);

        if (rc < 0) {
            write(wrfd, nack, sizeof nack);
        } else {
            write(wrfd, ack, sizeof ack);
        }

    } // while

out:
    if (buf) {
        free(buf);
    }

    //kill(getppid(), SIGUSR1);


    return 0;
}


