#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <signal.h>


#include "params.h"
#include "mysh_common.h"
#include "mysh_pipe.h"
#include "mysh_sysv.h"

// --------------------------------------------------------------------------
// mysh.c 
// a very basic unix shell program. This was developed as a homework assignment
// for the Linux Systems Programming class offered by UC Santa Cruz extension
// during summer quarter 2013.
// 
// this content and all of the files in this directory are
// Copyright (C) 2013, Bradley K. Selbrede, all rights reserved.



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


    signal(SIGUSR1, signal_handler);

    params = alloc_params();
    extract_params_from_cmdline_options(params, argc, argv);

    if (is_help_desired(params)) {
        show_help(argv[0]);
        goto done;
    }

    int rc;
    if (use_sysv_ipc(params)) {
        rc = mysh_implemented_with_sysv_ipc();
    } else {
        rc = mysh_implemented_with_pipe();
    }

done:
    free_params(params);

    return 0;
}
