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
write_hex(int fd, void *p)
{
    char buf[sizeof(p) * 2 + 1];
    static const char map[] = "0123456789abcdef";

    intptr_t pp = (intptr_t) p;
    char *cur = &buf[sizeof(buf) - 1];
    *cur = 0;

    for (size_t i = 0; i < sizeof(pp) * 2; i++) {
        cur--;
        *cur = map[pp & 0x0f];
        pp >>= 4;

        if (pp == 0) {
            break;
        }
    }

    write(fd, cur, (buf + sizeof(buf)) - cur);
}


void
sigusr1_handler(int sig, siginfo_t *info, void *ucontext)
{
    (void) ucontext;
    int arg = info->si_value.sival_int;

    WRITE_STATIC(STDOUT_FILENO, "sigusr1: ");
    write_hex(STDOUT_FILENO, (void *) (intptr_t) arg);
    WRITE_STATIC(STDOUT_FILENO, "\n");

    if (arg == 0) {
        sigqueue(getpid(), SIGUSR1, (union sigval) { 1 });

        int time = 2;
        do {
            time = sleep(time);
        } while (time > 0);

    } else if (arg != 1) {
        WRITE_STATIC(STDERR_FILENO, "error: unknown argument\n");
    }
    
    WRITE_STATIC(STDOUT_FILENO, "sigusr1: done\n");
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

    sigqueue(getpid(), SIGUSR1, (union sigval) { 0 });

    return 0;
}
