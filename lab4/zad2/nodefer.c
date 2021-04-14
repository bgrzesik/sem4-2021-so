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


void
sigusr1_handler(int sig, siginfo_t *info, void *ucontext)
{
    int arg = info->si_value.sival_int;

    WRITE_STATIC(STDOUT_FILENO, "SIGUSR1: ");
    write_dec(STDOUT_FILENO, arg);
    WRITE_STATIC(STDOUT_FILENO, "\n");

    if (arg == 0) {
        if (sigqueue(getpid(), SIGUSR1, (union sigval) { 1 }) != 0) {
            perror("sigqueue");
            exit(-1);
        }

        int time = 2;
        do {
            time = sleep(time);
        } while (time > 0);

    } else if (arg != 1) {
        WRITE_STATIC(STDERR_FILENO, "error: unknown argument\n");
    }
    
    WRITE_STATIC(STDOUT_FILENO, "SIGUSR1: done\n");
}


int
main(int argc, const char *argv[])
{
    struct sigaction act;
    act.sa_sigaction = &sigusr1_handler;

#ifndef NO_NODEFER
    act.sa_flags = SA_NODEFER | SA_SIGINFO;
#else
    act.sa_flags = SA_SIGINFO;
#endif

    sigemptyset(&act.sa_mask);
    if (sigaction(SIGUSR1, &act, NULL) != 0) {
        perror("signaction");
        return -1;
    }

    if (sigqueue(getpid(), SIGUSR1, (union sigval) { 0 }) != 0) {
        perror("sigqueue");
        return -1;
    }

    return 0;
}
