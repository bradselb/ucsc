#define _POSIX_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>


#include "inet.h"
#include "mysh_common.h"

sig_atomic_t g_terminate;

// --------------------------------------------------------------------------
void signal_handler(int nr)
{
    if (nr == SIGUSR1) {
        g_terminate = 1;
    }
}




int main(int argc, char* argv[])
{
    int sock;
    int rc;
    char* buf;
    int bufsize = 4096;

    signal(SIGUSR1, signal_handler);

    sock = -1;

    buf = malloc(bufsize);
    if (!buf) {
        goto out;
    }
    memset(buf, 0, bufsize);

    sock = inet_bind("localhost", "56789");
    if (sock < 0) {
        fprintf(stderr, "(%s:%d) %s(), inet_bind() returned: %d\n", __FILE__, __LINE__, __FUNCTION__, sock);
        goto out;
    }

    rc = inet_listen(sock, 10);
    if (rc < 0) {
        fprintf(stderr, "(%s:%d) %s(), listen() returned: %d\n", __FILE__, __LINE__, __FUNCTION__, rc);
        goto out;
    }

    while (!g_terminate) {
        int peer;
        peer = inet_accept(sock);
        if (peer > 0) {
            do_non_interactive_loop(peer, STDERR_FILENO);
            close(peer);
        }
    } // while

out:
    if (sock > 0) {
        close(sock);
    }

    if (buf) {
        free(buf);
    }
    return 0;
}

