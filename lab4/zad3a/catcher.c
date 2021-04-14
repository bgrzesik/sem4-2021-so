#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#define TAG "[catcher]"
#include "common.h"


size_t counter = 0;


void
sigmsg_handler(int sig, siginfo_t *info, void *ucontext)
{
    counter++;
}

void
sigend_handler(int sig, siginfo_t *info, void *ucontext)
{
    WRITE_STATIC(STDOUT_FILENO, "[catcher] caught ");
    write_dec(STDOUT_FILENO, counter);
    WRITE_STATIC(STDOUT_FILENO, "\n");

    int (*method)(int, pid_t, int) = &method_kill;
    pid_t sender = info->si_pid;

    if (info->si_code == SI_QUEUE) {
        method = &method_sigqueue;

        WRITE_STATIC(STDOUT_FILENO, "[catcher] supposed to receive ");
        write_dec(STDOUT_FILENO, info->si_value.sival_int);
        WRITE_STATIC(STDOUT_FILENO, "\n");
    }

    if (sender != 0) { 
        int msg_signal = SIGUSR1;
        int end_signal = SIGUSR2;
        
        if (sig == SIGRTMIN+1) {
            msg_signal = SIGRTMIN+0;
            end_signal = SIGRTMIN+1;
        }

        WRITE_STATIC(STDOUT_FILENO, "[catcher] responding to ");
        write_dec(STDOUT_FILENO, sender);
        WRITE_STATIC(STDOUT_FILENO, "\n");

        transmit(method, msg_signal, end_signal, sender, counter);
    }
    
    exit(0);
}

int
main(int argc, const char *argv[])
{
    struct sigaction act;

    /* msg handler */
    act.sa_sigaction = &sigmsg_handler;
    act.sa_flags = SA_SIGINFO;

    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGUSR2);
    sigaddset(&act.sa_mask, SIGRTMIN+1);

    if (sigaction(SIGUSR1, &act, NULL) != 0 || sigaction(SIGRTMIN+0, &act, NULL)) {
        perror("signaction");
        return -1;
    }

    /* end handler */
    act.sa_sigaction = &sigend_handler;
    act.sa_flags = SA_SIGINFO;

    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGUSR1);
    sigaddset(&act.sa_mask, SIGRTMIN+0);

    if (sigaction(SIGUSR2, &act, NULL) != 0 || sigaction(SIGRTMIN+1, &act, NULL)) {
        perror("signaction");
        return -1;
    }

    printf("[catcher] PID = %u\n", getpid());

    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGINT);

    sigprocmask(SIG_SETMASK, &set, NULL);
    
    sigdelset(&set, SIGUSR1);
    sigdelset(&set, SIGRTMIN+0);

    sigdelset(&set, SIGUSR2);
    sigdelset(&set, SIGRTMIN+1);

    while (1) {
        sigsuspend(&set);
    }

    return 0;
}

