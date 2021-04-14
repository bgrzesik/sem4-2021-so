#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "common.h"


static int recv_ack = 0;
static int last_msg = 0;

void
transmit_sigmsg_handler(int sig, siginfo_t *info, void *ucontext)
{
    if (sig != SIGUSR1 && sig != SIGRTMIN+0) {
        return;
    } 

    recv_ack = 1;
    if (info->si_code == SI_QUEUE && info->si_value.sival_int != last_msg) {
        WRITE_STATIC(STDERR_FILENO, TAG " wrong ack\n");
        exit(-1);
    }
}


int
transmit(int (*method)(int, pid_t, int), 
         int msg_signal, int end_signal, 
         pid_t catcher, int n)
{
    signal_block_all();

    WRITE_STATIC(STDOUT_FILENO, TAG " transmitting ");
    write_dec(STDOUT_FILENO, n);
    WRITE_STATIC(STDOUT_FILENO, " to ");
    write_dec(STDOUT_FILENO, catcher);
    WRITE_STATIC(STDOUT_FILENO, "\n");

    for (int i = 0; i < n; i++) {
        if (method(msg_signal, catcher, last_msg = i) != 0) {
            return -1;
        }

        /* msg handler */
        struct sigaction act;
        act.sa_sigaction = &transmit_sigmsg_handler;
        act.sa_flags = SA_SIGINFO;

        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask, SIGUSR2);
        sigaddset(&act.sa_mask, SIGRTMIN+1);
        
        recv_ack = 0;
        if (sigaction(SIGUSR1, &act, NULL) != 0 || sigaction(SIGRTMIN+0, &act, NULL)) {
            perror("sigaction");
            return -1;
        }

        sigset_t set;
        sigfillset(&set);
        sigdelset(&set, SIGINT);
        sigdelset(&set, SIGUSR1);
        sigdelset(&set, SIGRTMIN+0);

        while (!recv_ack) {
            sigsuspend(&set);
        }

        signal(SIGUSR1, SIG_DFL);
        signal(SIGRTMIN+0, SIG_DFL);
    }

    if (method(end_signal, catcher, n) != 0) {
        return -1;
    }

    return 0;
}


