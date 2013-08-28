#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>


#include "mysh_sysv.h"
#include "mysh_common.h"

int mysh_implemented_with_sysv_ipc()
{
    int rc = 0;
    fprintf(stderr, "sorry, this feature is not yet implmented. Please try --pipe\n");

    return rc;
}

