#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "common.h"

void
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


int
method_kill(int sig, pid_t pid, int n)
{
    if (kill(pid, sig) != 0) {
        WRITE_STATIC(STDERR_FILENO, TAG " ");
        perror("kill");
        return -1;
    }
    return 0;
}

int
method_sigqueue(int sig, pid_t pid, int n)
{
    if (sigqueue(pid, sig, (union sigval) n) != 0) {
        WRITE_STATIC(STDERR_FILENO, TAG " ");
        perror("sigqueue");
        return -1;
    }
    return 0;
}

int 
signal_block_all(void)
{
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGINT);

    if (sigprocmask(SIG_SETMASK, &set, NULL) != 0) {
        perror("sigprocmask");
        return -1;
    }

    return 0;
}
