#define MYSH_PROMPT "mysh> "

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


// --------------------------------------------------------------------------
void do_cmd(char* buf, int len);
int parse_cmd(char* buf, char** vbuf, int argcount);
int builtin_cmd(char** argv);
int exec_cmd(char** argv);
int printwaitstatus(FILE* wfp, int pid, int st);

// --------------------------------------------------------------------------
int g_terminate;

// --------------------------------------------------------------------------
int printwaitstatus(FILE* wfp, int pid, int st)
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
// returns: non-zero if this argv is a built-in command.
int builtin_cmd(char** argv)
{
    int st;
    int rc = 0;

    if ((0 == strncmp(argv[0], "exit", 4)) || (0 == strncmp(argv[0], "quit", 4))) {
        rc = 1; // yes, this was a built-in. 
        g_terminate = 1;
    } else if (0 == strncmp(argv[0], "cd", 2)) {
        rc = 1;
        if (argv[1] && (0 != (st = chdir(argv[1])))) {
            fprintf(stderr, "failed to change directory to '%s'\n", argv[1]);
            rc = -1;
        }
    } else if (0 == strncmp(argv[0], "hello", 5)) {
        fprintf(stderr, "\nHello! from process %d\n", getpid());
        rc = 1;
    } else {
        // this is not an internal command.
        rc = 0;
    }

    return rc;
}


// --------------------------------------------------------------------------
int exec_cmd(char** argv)
{
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork() returned: %d\n", pid);
        exit(-1);
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
        // printwaitstatus(stdout, pid, st);
    }
    return 0;
}


// --------------------------------------------------------------------------
int parse_cmd(char* buf, char** vbuf, int n)
{
    int i = 0;
    const char* delim = " ,\t\n";
    char* token;
    size_t len;

    token = strtok(buf, delim);
    while (token && i < n) {
        len = strlen(token);
        vbuf[i] = malloc(len + 1);
        strcpy(vbuf[i], token); // happily, strcpy() copies the terminating zero too.
        token = strtok(NULL, delim);
        ++i;
    }

    vbuf[i] = 0; // it should already be the case. 

    return i;
}


// --------------------------------------------------------------------------
void do_cmd(char* buf, int bufsize)
{
    int i;
    char* vbuf[32];
    int arg_count;
    int max_arg_count = sizeof vbuf / sizeof vbuf[0];

    memset(vbuf, 0, sizeof vbuf);

    arg_count = parse_cmd(buf, vbuf, max_arg_count - 1);
    //printf("(%s:%d) %s(),  arg_count: %d\n", __FILE__, __LINE__, __FUNCTION__, arg_count);

    if (arg_count < 1) {
        ; // nothing to do.
    } else if (builtin_cmd(vbuf)) {
        ; // we're done.
    } else {
        // an external command
        exec_cmd(vbuf);
    }

    // free the arg list
    for (i=0; i<arg_count; ++i) {
        if (vbuf[i]) {
            free(vbuf[i]);
        }
    }
}

// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
    char* prompt;
    FILE* rfp;
    char* buf;
    int bufsize;

    bufsize = 512;
    buf = malloc(bufsize);
    if (!buf) {
        goto out;
    }

    memset(buf, 0, sizeof buf);

    rfp = stdin;
    if (isatty(fileno(rfp))) {
        prompt = MYSH_PROMPT;
        fprintf(stderr, "%s", prompt);
    } else {
        prompt = "";
    }

    rfp = stdin;
    while (fgets(buf, bufsize, rfp)) {
        if (*buf) {
            do_cmd(buf, bufsize);
        }

        if (g_terminate) {
            break;
        }

        if (prompt) {
            fprintf(stderr, "%s", prompt);
        }
    }

out:
    if (buf) {
        free(buf);
    }

    return 0;
}
