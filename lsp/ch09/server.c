#define _POSIX_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

//#include <sys/socket.h>
//#include <arpa/inet.h>


#include "inet.h"

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
    int  len;
    char* buf;
    int bufsize = 4096;

    signal(SIGUSR1, signal_handler);

    sock = -1;

    buf = malloc(bufsize);
    if (!buf) {
        goto out;
    }
    memset(buf, 0, bufsize);

    sock = inet_bind("localhost", "16875");
    if (sock < 0) {
        fprintf(stderr, "(%s:%d) %s(), inet_bind() returned: %d\n", __FILE__, __LINE__, __FUNCTION__, sock);
        goto out;
    }

    rc = inet_listen(sock, 10);
    if (rc < 0) {
        fprintf(stderr, "(%s:%d) %s(), listen() returned: %d\n", __FILE__, __LINE__, __FUNCTION__, rc);
        goto out;
    }

    int peer;
    peer = inet_accept(sock);
    if (peer < 0) {
//        fprintf(stderr, "(%s:%d) %s(), accept() returned: %d\n", __FILE__, __LINE__, __FUNCTION__, peer);
//        goto out;
    } else {


    len = read(peer, buf, bufsize);
//    while (!g_terminate && len > 0) {
        write(peer, buf, len);
//        memset(buf, 0, bufsize);
//    }

    close(peer);
    }

out:
    if (sock > 0) {
        close(sock);
    }

    if (buf) {
        free(buf);
    }
    return 0;
}

