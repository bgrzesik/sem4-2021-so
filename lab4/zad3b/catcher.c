#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

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
    printf("[catcher] PID = %u\n", getpid());

    int n;
    if ((n = receive()) < 0) {
        return -1;
    }
    
    printf("[catcher] exit\n");
    return 0;
}

