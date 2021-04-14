#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define TAG "[sender]"
#include "common.h"

size_t counter = 0;
size_t n = 0;


void
sigmsg_handler(int sig, siginfo_t *info, void *ucontext)
{
    counter++;
}


void
sigend_handler(int sig, siginfo_t *info, void *ucontext)
{
    WRITE_STATIC(STDOUT_FILENO, "[sender] caught ");
    write_dec(STDOUT_FILENO, counter);
    WRITE_STATIC(STDOUT_FILENO, " but sent ");
    write_dec(STDOUT_FILENO, n);
    WRITE_STATIC(STDOUT_FILENO, "\n");

    exit(0);
}

int
main(int argc, const char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "usage: ./a.out <catcher PID> <n> kill|sigqueue|sigrt\n");
        return -1;
    }
    pid_t catcher = (pid_t) strtol(argv[1], NULL, 10);
    if (catcher == 0) {
        fprintf(stderr, "usage: ./a.out <catcher PID> <n> kill|sigqueue|sigrt\n");
        return -1;
    }

    n = (size_t) strtol(argv[2], NULL, 10);
    if (n == 0) {
        fprintf(stderr, "usage: ./a.out <catcher PID> <n> kill|sigqueue|sigrt\n");
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
        fprintf(stderr, "usage: ./a.out <catcher PID> <n> kill|sigqueue|sigrt\n");
        return -1;
    }

    struct sigaction act;
    act.sa_sigaction = &sigmsg_handler;
    act.sa_flags = SA_SIGINFO | SA_NODEFER;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, end_signal);
    if (sigaction(msg_signal, &act, NULL) != 0) {
        perror("signaction");
        return -1;
    }

    act.sa_sigaction = &sigend_handler;
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, msg_signal);
    if (sigaction(end_signal, &act, NULL) != 0) {
        perror("signaction");
        return -1;
    }

    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGINT);
    sigprocmask(SIG_SETMASK, &set, NULL);

    transmit(method, msg_signal, end_signal, catcher, n);

    sigdelset(&set, msg_signal);
    sigdelset(&set, end_signal);

    while (1) {
        sigsuspend(&set);
    }

    return 0;
}

