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
sigsegv_handler(int sig, siginfo_t *info, void *ucontext)
{
    WRITE_STATIC(STDOUT_FILENO, "Segmentation fault\nAddress: 0x");
    write_hex(STDOUT_FILENO, info->si_addr);
    WRITE_STATIC(STDOUT_FILENO, "\n");
    abort();
}


static void
kill_stack(void)
{
    char buf[8192];
    (void) buf;
    kill_stack();
}


int
main(int argc, const char *argv[])
{
    static char alt_stack[SIGSTKSZ * 4];
    stack_t sigstk;
    sigstk.ss_flags = 0;
    sigstk.ss_size = sizeof(alt_stack);
    sigstk.ss_sp = alt_stack;

    if (sigaltstack(&sigstk, NULL) != 0) {
        perror("sigaltstack");
        return -1;
    }

    struct sigaction act;
    act.sa_sigaction = &sigsegv_handler;

#ifndef NO_ONSTACK
    act.sa_flags = SA_ONSTACK | SA_SIGINFO;
#else
    act.sa_flags = SA_SIGINFO;
#endif

    sigemptyset(&act.sa_mask);
    if (sigaction(SIGSEGV, &act, NULL) != 0) {
        perror("signaction");
        return -1;
    }

    WRITE_STATIC(STDOUT_FILENO, "Killing stack\n");
    kill_stack();

    return 0;
}

