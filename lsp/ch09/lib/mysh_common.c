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
#include "tokenize.h"
#include "fileops.h"

#define MAX_ARGC 1024
#define MYSH_PROMPT "mysh> "
#define MYSH_BUFFER_SIZE 4096
#define CTRL_D 0x04



// --------------------------------------------------------------------------
static int is_quit_cmd(const char* s);
static int is_local_cmd(const char* s);
static int is_putfile_cmd(const char* s);

static int do_local_command(const char* tokbuf, size_t tokbufsize, int tokcount);
static int do_remote_command(const char* tokbuf, size_t tokbufsize, int fd);
static int do_putfile_command(const char* tokbuf, size_t tokbufsize, int destfd);

static int do_builtin_cmd(int argc, char** argv, int fd);
static int do_external_cmd(char** argv, int fd);

// private functions in other files
// these are called by do_builtin_command()
int cat(int argc, char** argv, int fd);
int wc(int argc, char** argv, int fd);
int ls(int argc, char** argv, int fd);





// --------------------------------------------------------------------------
// This function implements the interactive loop. That is, the user interacts
// directly with this function. The function reads input from stdin, tokenizes
// the command string and conditionally, sends the tokenized command string 
// to the server on the server fd. Then it waits for a reply from the server. 
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


    int terminate;
    terminate = 0;
    while (!terminate) {
        if (prompt) {
            fprintf(stdout, "%s", prompt);
            fflush(stdout);
        }

        // read a line of input from the user
        memset(buf, 0, bufsize); // so not to confuse tokenize()
        if (NULL == fgets(buf, bufsize, stdin)) {
            break;
        }

        // blank line? 
        if ('\n' == buf[0]) {
            continue;
        }



        // tokenize the command string in-place
        const char* delims = " \t\n,;:=";
        size_t tokbufsize = 0;
        int tokcount;


        tokcount = tokenize(buf, bufsize, delims, &tokbufsize);
        const char* tokbuf = (const char*)buf; // convenient sugar.

        //debug aids
        //fprintf(stderr, "\tokbufsize: %lu\n", tokbufsize);
        //show_tokenbuf(tokbuf, tokbufsize);

        terminate = is_quit_cmd(tokbuf);

        if (is_local_cmd(tokbuf)) {
            do_local_command(tokbuf, tokbufsize, tokcount);
            continue;
        }


        // send the tokenized command string to the server. 
        write(server_fd, tokbuf, tokbufsize);
        get_response_from_server(server_fd);

        if (is_putfile_cmd(tokbuf)) {
            // TODO: check return code.
            do_putfile_command(tokbuf, tokbufsize, server_fd);
        } 


    } // while !terminate

out:
    if (buf) {
        free(buf);
    }

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
        //fprintf(stderr, "(%s:%d) %s(), read() returned: %d [%s]\n", __FILE__, __LINE__, __FUNCTION__ , count, strerror(errno));



        if (0 == count) {
            // client hung-up.
            break;
        } else if (count < 0) {
            fprintf(stderr, "(%s:%d) %s(), read() returned: %d [%s]\n", __FILE__, __LINE__, __FUNCTION__ , count, strerror(errno));
            break;
        }

        // whomever ends up "doing" the command, gets to write 
        // the output back to the client. 
        do_remote_command(buf, count, client_fd);

        // and when that's done, we send a '^D' to tell
        // the client that we're done with this command. 
        buf[0] = CTRL_D;
        write(client_fd, buf, 1);
    }


out:
    if (buf) {
        free(buf);
    }

    return 0;
}


// --------------------------------------------------------------------------
int get_response_from_server(int server_fd)
{
    int rc = 0;
    int timeout;
    timeout = 10000; // milliseconds.

    struct pollfd ps;
    ps.fd = server_fd;
    ps.events = POLLIN;
    ps.revents = 0;

    char* buf;
    int bufsize;


    // allocate an input buffer.
    bufsize = MYSH_BUFFER_SIZE ;
    buf = malloc(bufsize);
    if (!buf) {
        rc = -1;
        goto out;
    }

    int done;
    done = 0;
    while (!done) {
        int poll_rc;

        // see if the server has something to say.
        poll_rc = poll(&ps, 1, timeout);

        if (0 == poll_rc) {
            // timed out.
            fprintf(stderr, "(%s:%d) %s(), poll() timeout [%s]\n", __FILE__, __LINE__, __FUNCTION__ , strerror(errno));
            done = 1;
        } else if (poll_rc < 0) {
            // an error.
            fprintf(stderr, "(%s:%d) %s(), poll() returned: %d [%s]\n", __FILE__, __LINE__, __FUNCTION__ , poll_rc, strerror(errno));
            done = 1;
        } else {
            ssize_t rc;
            
            // get the response from server
            memset(buf, 0, bufsize);
            rc = read(server_fd, buf, bufsize-1);
            if (rc > 0) {
                fprintf(stdout, "%s", buf);
                if (CTRL_D == buf[rc-1]) {
                    rc = 0; // sucess.
                    done = 1;
                }
            } else {
                fprintf(stderr, "(%s:%d) %s(), read() returned: %d [%s]\n", __FILE__, __LINE__, __FUNCTION__ , poll_rc, strerror(errno));
                done = 1;
            }
        } 
    } // while !done

out:
    if(buf) {
        free(buf);
    }
    return rc;
}



// --------------------------------------------------------------------------
int is_quit_cmd(const char* buf)
{
    int v;
    v = buf && (0 == strncmp(buf, "exit", 4) || 0 == strncmp(buf, "quit", 4));
    return v;
}

// --------------------------------------------------------------------------
int is_local_cmd(const char* buf)
{
    int v;
    v = buf && (0 == strncmp(buf, "local", 5));
    return v;
}

// --------------------------------------------------------------------------
int is_putfile_cmd(const char* buf)
{
    int v;
    v = buf && (0 == strncmp(buf, "put", 3));
    return v;
}


// --------------------------------------------------------------------------
int do_local_command(const char* tokbuf, size_t tokbufsize, int tokcount)
{
    int rc;
    int argc;
    char** argv;

    rc = -1;
    argc = 0;
    argv = 0;

    // need at least two tokens for this to be interesting.
    if (tokcount < 2) {
        rc = 0;
        goto out;
    }

    // need to make SURE that argv has room for null terminator.
    argv = malloc((tokcount+1) * sizeof(char*));
    if (!argv) {
        rc = -1;
        goto out;
    }
    memset(argv, 0, sizeof argv);

    // convert tokens to argv
    argc = init_argv_from_tokenbuf(argv, tokbuf, tokbufsize, tokcount);
    if (argc != tokcount) {
        // lies!
        rc = -1;
        goto out;
    }

    // keeping in mind that first token is 'local',
    // doctor up what we say to the do_XXX_cmd() functions.
    rc = do_builtin_cmd(argc - 1, &argv[1], STDOUT_FILENO);
    if (0 != rc) {
        rc = do_external_cmd(&argv[1], STDOUT_FILENO);
    }

out:
    if (argv) {
        free(argv);
    }

    return rc;
}

// --------------------------------------------------------------------------
int do_remote_command(const char* tokbuf, size_t tokbufsize, int fd)
{
    int rc;
    int tokcount;
    int argc;
    char** argv;

    rc = -1;

    tokcount = count_tokens(tokbuf, tokbufsize);

    // need to make SURE that argv has room for null terminator.
    argv = malloc((tokcount+1) * sizeof(char*));
    if (!argv) {
        rc = -1;
        goto out;
    }
    memset(argv, 0, sizeof argv);

    // convert tokens to argv
    argc = init_argv_from_tokenbuf(argv, tokbuf, tokbufsize, tokcount);
    if (argc != tokcount) {
        // lies!
        fprintf(stderr, "(%s:%d) %s(), argc: %d != tokcount: %d\n", __FILE__, __LINE__, __FUNCTION__, argc, tokcount);
        rc = -1;
        goto out;
    }

    rc = do_builtin_cmd(argc, argv, fd);

out:
    if (argv) {
        free(argv);
    }

    return rc;
}

// --------------------------------------------------------------------------
// form of command in the token buffer is expected to be: 
// put <local file path> <remote filename>
// open local file,
// read upto 512 bytes, encode the data bytes and send a message of the form:
// writefiledata <remote filename> <offset> <encoded data>
int do_putfile_command(const char* tokbuf, size_t tokbufsize, int destfd)
{
    int rc = 0;
    int tokcount;
    int argc;
    char** argv = 0;

    rc = -1;

    tokcount = count_tokens(tokbuf, tokbufsize);
    //fprintf(stderr, "(%s:%d) %s(), tokcount: %d\n", __FILE__, __LINE__, __FUNCTION__, tokcount);

    // need three args for this to be interesting.
    if (tokcount < 3) {
        fprintf(stderr, "(%s:%d) %s(), tokcount: %d < 3\n", __FILE__, __LINE__, __FUNCTION__, tokcount);
        rc = -1;
        goto out;
    }

    // need to make SURE that argv has room for null terminator.
    argv = malloc((tokcount+1) * sizeof(char*));
    if (!argv) {
        rc = -1;
        goto out;
    }
    memset(argv, 0, sizeof argv);

    // convert tokens to argv
    argc = init_argv_from_tokenbuf(argv, tokbuf, tokbufsize, tokcount);
    if (argc != tokcount) {
        // lies!
        fprintf(stderr, "(%s:%d) %s(), argc: %d != tokcount: %d\n", __FILE__, __LINE__, __FUNCTION__, argc, tokcount);
        rc = -1;
        goto out;
    }

    rc = send_putfile_messages(argv[1], argv[2], destfd);
    if (0 != rc) {
    }

out:
    if (argv) {
        free(argv);
    }

    return rc;
}



// --------------------------------------------------------------------------
// returns: non-zero if this argv is a built-in command.
int do_builtin_cmd(int argc, char** argv, int fd)
{
    int rc;
    int ec;
    char* buf = 0;
    int bufsize = 512;
    int length;

    // a small buffer for a reply message - if necessary. 
    buf = malloc(bufsize);
    if (!buf) {
        rc = -1;
        goto out;
    }
    memset(buf, 0, bufsize);


    // show the args that we've received. helps with debug.
    //for(int i=0; i<argc; ++i) {
    //    fprintf(stderr, "%s ", argv[i]);
    //}
    //fprintf(stderr, "\n");



    rc = 0;
    length = 0;
    if (is_quit_cmd(argv[0])) {
        length = snprintf(buf, bufsize, "%s.  OK. Good-bye from pid: %d\n", argv[0], getpid());

    } else if (0 == strncmp(argv[0], "hello", 5)) {
        length = snprintf(buf, bufsize, "hello from pid: %d\n", getpid());

    } else if (0 == strncmp(argv[0], "cd", 2)) {
        if (argv[1] && (0 != (ec = chdir(argv[1])))) {
            length = snprintf(buf, bufsize, "failed to change directory to '%s'\n", argv[1]);
        }
    } else if (0 == strncmp(argv[0], "ls", 2)) {
        ls(argc, argv, fd);

    } else if (0 == strncmp(argv[0], "cat", 3)) {
        cat(argc, argv, fd);

    } else if (0 == strncmp(argv[0], "wc", 2)) {
        wc(argc, argv, fd);

    } else if (0 == strncmp(argv[0], "put", 3) && 3 == argc) {
        // remove existing file. 
        // TODO: be less drastic. 
        unlink(argv[2]);

    } else if (0 == strncmp(argv[0], "writefiledata", 13)) {
        writefiledata(argc, argv, fd);

    } else {
        // this is not an internal command.
        rc = 1;
    }

    if (length > 0) {
        // somebody here wanted to say something.
        write(fd, buf, length);
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


