
//#define _GNU_SOURCE

//#define DEBUG_CMDLINE_OPTIONS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include "params.h"


// --------------------------------------------------------------------------
static const char* DEFAULT_HOST = "0"; // means "any suitable interface"
static const char* DEFAULT_PORT = "56789";

// --------------------------------------------------------------------------
struct params
{
    const char* host_addr;
    const char* port_nr;
    int help;
    int debug;

    const char* log_file_name;

};

static const char* short_options = "a:p:hdl:";
static struct option long_options[] = {
    {.val='a', .name="host-name", .has_arg=required_argument, .flag=0},
    {.val='a', .name="ip-address", .has_arg=required_argument, .flag=0},
    {.val='p', .name="port-number", .has_arg=required_argument, .flag=0},
    {.val='h', .name="help", .has_arg=no_argument, .flag=0},
    {.val='d', .name="debug", .has_arg=no_argument, .flag=0},

    {.val='l', .name="lf", .has_arg=required_argument, .flag=0},
    {.val='l', .name="log-file", .has_arg=required_argument, .flag=0},

    {0,0,0,0}
};


// --------------------------------------------------------------------------
static int update_params_with_opt(struct params*, int opt, const char* arg);



// --------------------------------------------------------------------------
struct params* alloc_params(void)
{
    struct params* params;
    params = malloc(sizeof(struct params));
    if (params != 0) {
        memset(params, 0, sizeof *params);
        params->host_addr = DEFAULT_HOST;
        params->port_nr = DEFAULT_PORT;
    }
    return params;
}



// --------------------------------------------------------------------------
int extract_params_from_cmdline_options(struct params* params, int argc, char* argv[])
{
    int rc = 0;

    if (!params) {
        rc = -1;
        goto out;
    }

    int optc;
    while(-1 != (optc = getopt_long(argc, argv, short_options, long_options, 0))) {
        if ('?' == optc) {
            optc = optopt;
        }
        update_params_with_opt(params, optc, optarg);
    }

out: 
    return rc;
}


// --------------------------------------------------------------------------
int update_params_with_opt(struct params* params, int opt, const char* arg)
{
    int rc = 0;

#if defined DEBUG_CMDLINE_OPTIONS
    printf("option: %c", opt);
    if (arg) {
        printf(", value: %s", arg);
    }
    printf("\n");
#endif

    if (!params) {
        rc = -1;
        goto out;
    }
 
    switch (opt) {
        case 'a':
            params->host_addr = arg;
            break;

        case 'p':
            params->port_nr = arg;
            break;

        case 'h':
            params->help = 1;
            break;

        case 'd':
            params->debug = 1;
            break;

        case 'l':
            params->log_file_name = arg;
            break;

        default:
            // getopt_long() will print an error message
            break;
    }

out:
    return rc;
}


// --------------------------------------------------------------------------
void free_params(struct params* params)
{
    if (params != 0) {
        free(params);
    }
}

// --------------------------------------------------------------------------
const char* hostname(struct params* params)
{
    const char* name = 0;
    if (params) {
        name = params->host_addr;
    }
    return name;
}


// --------------------------------------------------------------------------
const char* portnumber(struct params* params)
{
    const char* nr = 0;
    if (params) {
        nr = params->port_nr;
    }
    return nr;
}

// --------------------------------------------------------------------------
int is_help_desired(struct params* params)
{
    int v;
    v = (params && params->help);
    return v;
}

// --------------------------------------------------------------------------
int is_debug_enabled(struct params* params)
{
    int v;
    v = (params && params->debug);
    return v;
}


// --------------------------------------------------------------------------
const char* log_file_name(struct params* params)
{
    const char* name = 0;
    if (params) {
        name = params->log_file_name;
    }
    return name;
}

// --------------------------------------------------------------------------
// this function needs to be kept in synch with the parameters
void show_help(const char* argv0)
{
    printf("My client application\n");
    printf("a very basic remote shell written as a homework assignment for the Linux Systems Programming class\n");
    printf("Usage: %s [options]\n", argv0);
    //printf("\n");
    //printf("\n");
    printf("\t-a, --host-name, \tthe network (ipv4) address or name of the server.\n");
    printf("\t    --ip-address\n");
    printf("\t-p, --port-number \tthe port number of the server application\n");
    printf("\t-h, --help \t\tprint this help message and exit.\n");
    printf("Examples:\n");
    printf("\tclient --host server.mydomain.org --port 13874\n");
    printf("\tclient --host localhost --port 12383\n");
    printf("\tclient --host 192.168.1.147  --port 17628\n");
    printf("Hints:\n");
    printf("\tStart the server first.\n");
    printf("\tBoth client and server must use the same port number\n");
    printf("\n");
}




