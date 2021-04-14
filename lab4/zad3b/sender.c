#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "common.h"


static void
print_usage(void)
{
    fprintf(stderr, "usage: ./a.out <catcher PID> <n> kill|sigqueue|sigrt\n");
}

int
main(int argc, const char *argv[])
{
    if (argc != 4) {
        print_usage();
        return -1;
    }

    pid_t catcher = (pid_t) strtol(argv[1], NULL, 10);
    if (catcher == 0) {
        print_usage();
        return -1;
    }

    size_t n = (size_t) strtol(argv[2], NULL, 10);
    if (n == 0) {
        print_usage();
        return -1;
    }

    int msg_signal = SIGUSR1;
    int end_signal = SIGUSR2;
    int (*method)(int, pid_t, int);

    if (strcmp(argv[3], "KILL") == 0 || strcmp(argv[3], "kill") == 0) {
        method = &method_kill;
    } else if (strcmp(argv[3], "SIGQUEUE") == 0 || strcmp(argv[3], "sigqueue") == 0) {
        method = &method_sigqueue;
    } else if (strcmp(argv[3], "SIGRT") == 0 || strcmp(argv[3], "sigrt") == 0) {
        msg_signal = SIGRTMIN+0;
        end_signal = SIGRTMIN+1;
        method = &method_kill;
    } else {
        print_usage();
        return -1;
    }

    transmit(method, msg_signal, end_signal, catcher, n);

    int recv;
    if ((recv = receive()) < 0) {
        return -1;
    }

    printf("[sender] caught %d and send %ld\n", recv, n);

    printf("[sender] exit\n");
    return 0;
}

