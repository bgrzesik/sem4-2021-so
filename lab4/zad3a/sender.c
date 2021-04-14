#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>


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
size_t n = 0;

void
sigmsg_handler(int sig, siginfo_t *info, void *ucontext)
{
    counter++;
}

void
sigend_handler(int sig, siginfo_t *info, void *ucontext)
{
    WRITE_STATIC(STDOUT_FILENO, "[sender] caugth ");
    write_dec(STDOUT_FILENO, counter);
    WRITE_STATIC(STDOUT_FILENO, " but sent ");
    write_dec(STDOUT_FILENO, n);
    WRITE_STATIC(STDOUT_FILENO, "\n");
    exit(0);
}

static int
method_kill(int msg_signal, int end_signal, pid_t catcher, int n)
{
    for (int i = 0; i < n; i++) {
        if (kill(catcher, msg_signal) != 0) {
            perror("kill");
            return -1;
        }
    }

    if (kill(catcher, end_signal) != 0) {
        perror("kill");
        return -1;
    }

    return 0;
}

static int
method_sigqueue(int msg_signal, int end_signal, pid_t catcher, int n)
{
    for (int i = 0; i < n; i++) {
        if (sigqueue(catcher, msg_signal, (union sigval) i) != 0) {
            perror("sigqueue");
            return -1;
        }
    }

    if (sigqueue(catcher, end_signal, (union sigval) n) != 0) {
        perror("sigqueue");
        return -1;
    }

    return 0;
}

int
main(int argc, const char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "usage: ./a.out <catcher PID> <n> kill|sigqueue|sigrt\n");
        return -1;
    }

    int msg_signal = SIGUSR1;
    int end_signal = SIGUSR2;

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

    /* printf("catcher = %d\n", catcher); */

    int (*method)(int, int, pid_t, int);
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

    if (method(msg_signal, end_signal, catcher, n) != 0) {
        return -1;
    }
    

    sigdelset(&set, msg_signal);
    sigdelset(&set, end_signal);

    while (1) {
        sigsuspend(&set);
    }

    return 0;
}

