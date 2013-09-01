#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <signal.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
//#include <netdb.h>
//#include <arpa/inet.h>

#include "params.h"
//#include "mysh_common.h"
#include "inet.h"

// --------------------------------------------------------------------------
// client.c 
// a very basic unix shell program that can act as a remote shell. It uses
// a tcp/ip socket to communicate with a server. The server can be running on
// the local host or it can run on a remote host. There is a very simple
// file transfer capability - making this something like the start of an 
// ftp client and server.
//
// This was developed as a homework assignment for the 
// Linux Systems Programming class offered by UC Santa Cruz extension
// during summer quarter 2013.
// 
// this content and all of the files in this directory are
// Copyright (C) 2013, Bradley K. Selbrede
//

// --------------------------------------------------------------------------
sig_atomic_t g_terminate;


// --------------------------------------------------------------------------
void signal_handler(int nr)
{
    if (nr == SIGUSR1) {
        g_terminate = 1;
    }
}


// --------------------------------------------------------------------------
int main(int argc, char** argv)
{
    struct params* params = 0;
    char* buf;
    size_t bufsize = 4096;
    size_t buflen = 0; // how many chars in buf.
    int sockfd = -1;

    buf = malloc(bufsize);
    if (!buf) {
        goto out;
    }
    memset(buf, 0, bufsize);


    signal(SIGUSR1, signal_handler);

    params = alloc_params();
    extract_params_from_cmdline_options(params, argc, argv);

    if (is_help_desired(params)) {
        show_help(argv[0]);
        goto out;
    }

    sockfd = inet_connect(hostname(params), portnumber(params), SOCK_STREAM);
    if (sockfd < 0) {
        fprintf(stderr, "failed to connect to remote host.\n");
        goto out;
    }

    buflen = 0;
    buflen += snprintf(buf+buflen, bufsize-buflen, "GET / HTTP/1.0\r\n");
    buflen += snprintf(buf+buflen, bufsize-buflen, "\r\n");
    buflen += snprintf(buf+buflen, bufsize-buflen, "Host: %s\r\n", hostname(params));
    buflen += snprintf(buf+buflen, bufsize-buflen, "From: bks62464@gmail.com\r\n");
    buflen += snprintf(buf+buflen, bufsize-buflen, "User-Agent: Brad's-client/0.01\r\n");
    buflen += snprintf(buf+buflen, bufsize-buflen, "\r\n");

    fputs(buf, stderr);

    ssize_t count;
    count = write(sockfd, buf, buflen);

    //shutdown(sockfd, SHUT_WR);

    fprintf(stderr, "wrote %ld bytes to the server\n", count);

    memset(buf, 0 , bufsize);
    while (0 < (count = read(sockfd, buf, bufsize))) {
        fprintf(stderr, "read %ld bytes from the server\n", count);
        fputs(buf, stderr);
        memset(buf, 0 , bufsize);
    }


out:
    if (sockfd > 0) {
        close(sockfd);
    }

    free_params(params);

    if  (buf) {
        free(buf);
    }

    return 0;
}


