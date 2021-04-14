#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>


#define WRITE_STATIC(fd, text)                  \
    do {                                        \
        static const char msg[] = (text);       \
        write((fd), msg, sizeof(msg));          \
    } while (0)


static inline void
write_dec(int fd, size_t pp)
{
    char buf[sizeof(pp) * 2 + 1];
    static const char map[] = "0123456789";

    char *cur = &buf[sizeof(buf) - 1];
    *cur = 0;

    size_t iter = ((size_t) log10(pp)) + 1;
    if (pp == 0) {
        iter = 1;
    }

    for (size_t i = 0; i < iter; i++) {
        cur--;
        *cur = map[pp % 10];
        pp /= 10;
    }

    write(fd, cur, (buf + sizeof(buf)) - cur);
}

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

    pid_t sender = info->si_pid;

    /* "When delivering a signal with a SA_SIGINFO handler, the kernel
        does not always provide meaningful values for all of the fields
        of the siginfo_t that are relevant for that signal." 
        sigaction(2) */
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

        for (int i = 0; i < counter; i++) {
            sigqueue(sender, msg_signal, (union sigval) i);
        }
        sigqueue(sender, end_signal, (union sigval) 0);
    }
    
    exit(0);
}

int
main(int argc, const char *argv[])
{
    struct sigaction act;
    act.sa_sigaction = &sigmsg_handler;
    act.sa_flags = SA_SIGINFO;

    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGUSR2);
    sigaddset(&act.sa_mask, SIGRTMIN+1);

    if (sigaction(SIGUSR1, &act, NULL) != 0 || sigaction(SIGRTMIN+0, &act, NULL)) {
        perror("signaction");
        return -1;
    }

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

