#define _POSIX_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>


#include "params.h"
#include "mysh_common.h"
#include "inet.h"


// --------------------------------------------------------------------------
sig_atomic_t g_terminate;

// --------------------------------------------------------------------------
void signal_handler(int nr)
{
    g_terminate = 1;
}



// --------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    int sock;
    int rc;
    char* buf;
    int bufsize = 4096;
    struct params* params = 0;
    int backlog = 10;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGUSR1, signal_handler);

    params = alloc_params();
    extract_params_from_cmdline_options(params, argc, argv);

    sock = -1;

    buf = malloc(bufsize);
    if (!buf) {
        goto out;
    }
    memset(buf, 0, bufsize);

    sock = inet_bind(hostname(params), portnumber(params));
    if (sock < 0) {
        fprintf(stderr, "(%s:%d) %s(), inet_bind() returned: %d\n", __FILE__, __LINE__, __FUNCTION__, sock);
        goto out;
    }

    rc = inet_listen(sock, backlog);
    if (rc < 0) {
        fprintf(stderr, "(%s:%d) %s(), listen() returned: %d\n", __FILE__, __LINE__, __FUNCTION__, rc);
        goto out;
    }

    fprintf(stderr, "pid: %d\n", getpid());

    while (!g_terminate) {
        int peer;
        peer = inet_accept(sock);
        if (peer > 0) {
            do_non_interactive_loop(peer, STDERR_FILENO);
            close(peer);
        } else {
            //g_terminate = 1;
            break;
        }
    } // while

    fprintf(stderr, "good-bye.\n");

out:
    if (sock > 0) {
        close(sock);
    }

    if (buf) {
        free(buf);
    }

    free_params(params);

    return 0;
}

