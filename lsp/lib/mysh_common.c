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
#define MYSH_PROMPT "mysh> "
#define MYSH_BUFFER_SIZE 4096

// this is a hack...fix it.
extern sig_atomic_t g_terminate;

// --------------------------------------------------------------------------
// this function implements the interactive loop. That is, the user interacts
// directly with this function. The function reads input from the user fd
// and sends it to the server on the server fd. Then it waits fro a reply from
// from the server. 
int do_interactive_loop(int server_fd)
{
    char* prompt = 0;
    char* buf;
    int bufsize;


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
            fprintf(stdout, "%s", prompt);
            fflush(stdout);
        }

        // read a line of input from the user
        if (NULL == fgets(buf, bufsize, stdin)) {
            break;
        }

        if ('\n' == buf[0]) {
            continue;
        }

        if (0 == strncmp(buf, "exit", 4)  || 0 == strncmp(buf, "quit", 4)) {
            g_terminate = 1;
        }

        // what I want to do...(but am not yet doing)
        // if this command string is a local command, 
        //     deal with it and continue to top of loop
        // otherwise...
        // Package the command string into a command message packet
        // and send the message packet to the server.
        // Then, use poll with a timeout (as we do now) to wait a bit for the server to say
        // something. When the server has something to say, read packets...how many? 


        // send the command string to the server. 
        write(server_fd, buf, strnlen(buf, bufsize));

    { // local scope -- think about refactoring this.
        int timeout;
        timeout = 10000; // milliseconds.

        struct pollfd ps;
        ps.fd = server_fd;
        ps.events = POLLIN;
        ps.revents = 0;

        int done;
        done = 0;
        while (!done) {
            int poll_rc;

            // see if the server has something to say.
            poll_rc = poll(&ps, 1, timeout);

            if (0 == poll_rc) {
                // timed out.
                fprintf(stderr, "(%s:%d) %s(), poll() timeout [%s]\n", __FILE__, __LINE__, __FUNCTION__ , strerror(errno));
                break;
            } else if (poll_rc < 0) {
                // an error.
                fprintf(stderr, "(%s:%d) %s(), poll() returned: %d [%s]\n", __FILE__, __LINE__, __FUNCTION__ , poll_rc, strerror(errno));
                break;
            } else {
                ssize_t rc;
                
                // get the response from server
                memset(buf, 0, bufsize);
                rc = read(server_fd, buf, bufsize-1);
                if (rc > 0) {
                    fprintf(stdout, "%s", buf);
                    if (buf[rc-1] == 0) {
                        done = 1;
                    }
                } else {
                    fprintf(stderr, "(%s:%d) %s(), read() returned: %d [%s]\n", __FILE__, __LINE__, __FUNCTION__ , poll_rc, strerror(errno));
                    done = 1;
                }
            } 
        } // while !done
    } // end local scope

    } // while !g_terminate

out:
    if (buf) {
        free(buf);
    }

    //fprintf(stderr, "%s(), g_terminate: %d\n", __FUNCTION__, g_terminate);

    return 0;
}


// --------------------------------------------------------------------------
// on server side, we read stuff from the client-fd. Normally, the client-fd
// is the socket fd that we got from a call to accept(). We can log stuff
// to the log-fd. 
int do_non_interactive_loop(int client_fd, int log_fd)
{
    char* buf;
    int bufsize;


    // allocate an input buffer.
    bufsize = MYSH_BUFFER_SIZE ;
    buf = malloc(bufsize);
    if (!buf) {
        kill(getpid(), SIGUSR1);
        goto out;
    }


    while (1) {
        int count;

        memset(buf, 0, bufsize);
        count = read(client_fd, buf, bufsize-1);

        if (0 == count) {
            // client hung-up.
            break;
        } else if (count < 0) {
            fprintf(stderr, "(%s:%d) %s(), read() returned: %d [%s]\n", __FILE__, __LINE__, __FUNCTION__ , count, strerror(errno));
            break;
        }

        // whomever ends up "doing" the command, gets to write 
        // the output back to the client. 
        do_cmd(buf, bufsize, client_fd);

        // and when that's done we send a little something to 
        // tell the client that we're done with this command. 
        buf[0] = 0;
        write(client_fd, buf, 1);
    }


out:
    if (buf) {
        free(buf);
    }

    return 0;
}




// --------------------------------------------------------------------------
int do_cmd(char* buf, int bufsizey, int fd)
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
    
    rc = do_built_in_cmd(argc, argv, fd);
// not ready yet.
//    if (1 == rc) {
//        rc = do_external_cmd(argv, fd);
//    }

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
int do_built_in_cmd(int argc, char** argv, int fd)
{
    int rc;
    int ec;
    char* buf = 0;
    int bufsize = 512;
    int length;

    buf = malloc(bufsize);
    if (!buf) {
        rc = -1;
        goto out;
    }
    memset(buf, 0, bufsize);


    rc = 0;
    if ((0 == strncmp(argv[0], "exit", 4)) || (0 == strncmp(argv[0], "quit", 4))) {
        length = snprintf(buf, bufsize, "quit...OK. Good-bye from pid: %d\n", getpid());
        write(fd, buf, length);
        //kill(getpid(), SIGUSR1);

    } else if (0 == strncmp(argv[0], "cd", 2)) {
        if (argv[1] && (0 != (ec = chdir(argv[1])))) {
            fprintf(stderr, "failed to change directory to '%s'\n", argv[1]);
        }
    } else if (0 == strncmp(argv[0], "ls", 2)) {
        ls(argc, argv, fd);

    } else if (0 == strncmp(argv[0], "cat", 3)) {
        cat(argc, argv, fd);

    } else if (0 == strncmp(argv[0], "wc", 2)) {
        wc(argc, argv, fd);

    } else if (0 == strncmp(argv[0], "hello", 5)) {
        length = snprintf(buf, bufsize, "hello from pid: %d\n", getpid());
        write(fd, buf, length);

    } else {
        // this is not an internal command.
        length = snprintf(buf, bufsize, "Sorry, I do not understand '%s' yet.\n", argv[0]);
        write(fd, buf, length);
        rc = 1;
    }

out:
    if(buf) {
        free(buf);
    }

    return rc;
}


// --------------------------------------------------------------------------
int do_external_cmd(char** argv, int fd)
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
            fprintf(stderr, "(%s:%d) %s(), execvp(%s) returned: %d\n", __FILE__, __LINE__, __FUNCTION__ , argv[0], ec);
            _exit(errno);  // terminate the child but, do not mess up parent
        }

    } else {
        // parent process just waits for child to complete.
        int st;
        waitpid(pid, &st, 0);
    }

    return 0;
}


