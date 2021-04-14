#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "common.h"


static int counter = 0;
static int recv_end = 0;

static int (*method)(int, pid_t, int) = &method_kill;
static int msg_signal = SIGUSR1;
static int end_signal = SIGUSR2;


void
recv_sigmsg_handler(int sig, siginfo_t *info, void *ucontext)
{
    if (sig == SIGRTMIN+0) {
        msg_signal = SIGRTMIN+0;
        end_signal = SIGRTMIN+1;
    } else if (sig == SIGUSR1) {
        msg_signal = SIGUSR1;
        end_signal = SIGUSR2;
    }

    counter++;

    if (info->si_code == SI_QUEUE) {
        method = &method_sigqueue;
    } else if (info->si_code == SI_USER) {
        method = &method_kill;
    }

    /* send ack */
    pid_t sender = info->si_pid;
    if (sender != 0) { 
        if (method(msg_signal, sender, info->si_value.sival_int) != 0) {
            exit(-1);
        }
    }
}

void
recv_sigend_handler(int sig, siginfo_t *info, void *ucontext)
{
    recv_end = 1;

    WRITE_STATIC(STDOUT_FILENO, TAG " caught ");
    write_dec(STDOUT_FILENO, counter);
    WRITE_STATIC(STDOUT_FILENO, "\n");

    if (info->si_code == SI_QUEUE) {
        method = &method_sigqueue;

        WRITE_STATIC(STDOUT_FILENO, TAG " supposed to receive ");
        write_dec(STDOUT_FILENO, info->si_value.sival_int);
        WRITE_STATIC(STDOUT_FILENO, "\n");
    }
    
    if (sig == SIGRTMIN+1) {
        msg_signal = SIGRTMIN+0;
        end_signal = SIGRTMIN+1;
    }

#ifdef CATCHER
    pid_t sender = info->si_pid;

    if (sender != 0) { 
        WRITE_STATIC(STDOUT_FILENO, TAG " responding to ");
        write_dec(STDOUT_FILENO, sender);
        WRITE_STATIC(STDOUT_FILENO, "\n");

        transmit(method, msg_signal, end_signal, sender, counter);
    }
#endif
}

int
receive(void)
{
    signal_block_all();

    struct sigaction act;

    /* msg handler */
    act.sa_sigaction = &recv_sigmsg_handler;
    act.sa_flags = SA_SIGINFO;

    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGUSR2);
    sigaddset(&act.sa_mask, SIGRTMIN+1);

    if (sigaction(SIGUSR1, &act, NULL) != 0 || sigaction(SIGRTMIN+0, &act, NULL) != 0) {
        perror("signaction");
        return -1;
    }

    /* end handler */
    act.sa_sigaction = &recv_sigend_handler;
    act.sa_flags = SA_SIGINFO;

    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGUSR1);
    sigaddset(&act.sa_mask, SIGRTMIN+0);

    recv_end = 0;
    if (sigaction(SIGUSR2, &act, NULL) != 0 || sigaction(SIGRTMIN+1, &act, NULL) != 0) {
        perror("signaction");
        return -1;
    }

    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGUSR1);
    sigdelset(&set, SIGRTMIN+0);

    sigdelset(&set, SIGUSR2);
    sigdelset(&set, SIGRTMIN+1);

    while (!recv_end) {
        sigsuspend(&set);
    }

    signal(SIGUSR1, SIG_DFL);
    signal(SIGRTMIN+0, SIG_DFL);

    signal(SIGUSR2, SIG_DFL);
    signal(SIGRTMIN+1, SIG_DFL);

    return counter;
}

