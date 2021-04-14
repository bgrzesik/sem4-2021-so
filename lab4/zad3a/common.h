#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>


#ifndef __COMMON_H__
#define __COMMON_H__


#ifndef TAG
#error "TAG must me defined"
#endif


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


static int
method_kill(int sig, pid_t pid, int n)
{
    if (kill(pid, sig) != 0) {
        WRITE_STATIC(STDERR_FILENO, TAG);
        perror("kill");
        return -1;
    }
    return 0;
}


static int
method_sigqueue(int sig, pid_t pid, int n)
{
    if (sigqueue(pid, sig, (union sigval) n) != 0) {
        WRITE_STATIC(STDERR_FILENO, TAG);
        perror("sigqueue");
        return -1;
    }
    return 0;
}


static int
transmit(int (*method)(int, pid_t, int), 
         int msg_signal, int end_signal, 
         pid_t catcher, int n)
{
    WRITE_STATIC(STDOUT_FILENO, TAG " transmitting ");
    write_dec(STDOUT_FILENO, n);
    WRITE_STATIC(STDOUT_FILENO, " to ");
    write_dec(STDOUT_FILENO, catcher);
    WRITE_STATIC(STDOUT_FILENO, "\n");

    for (int i = 0; i < n; i++) {
        if (method(msg_signal, catcher, i) != 0) {
            return -1;
        }
    }

    if (method(end_signal, catcher, n) != 0) {
        return -1;
    }

    return 0;
}


#endif
