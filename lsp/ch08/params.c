
//#define _GNU_SOURCE


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <getopt.h>

#include "params.h"

// --------------------------------------------------------------------------
struct params
{
    int pipe;
    int sysv;
    int help;
    int debug;

    const char* log_file_name;

};

static const char* short_options = "pihdl:";
static struct option long_options[] = {
    {.val='p', .name="pipe", .has_arg=no_argument, .flag=0},
    {.val='i', .name="sysv", .has_arg=no_argument, .flag=0},
    {.val='i', .name="ipc", .has_arg=no_argument, .flag=0},
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
        params->pipe = 1;
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
        case 'p':
            params->pipe = 1;
            params->sysv = 0;
            break;

        case 'i':
            params->sysv = 1;
            params->pipe = 0;
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
int use_pipe(struct params* params)
{
    int v;
    v = (params && params->pipe);
    return v;
}


// --------------------------------------------------------------------------
int use_sysv_ipc(struct params* params)
{
    int v;
    v = (params && params->sysv);
    return v;
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



