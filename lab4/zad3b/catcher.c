#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "common.h"

int
main(int argc, const char *argv[])
{
    printf("[catcher] PID = %u\n", getpid());

    int n;
    if ((n = receive()) < 0) {
        return -1;
    }
    
    printf("[catcher] exit\n");
    return 0;
}

