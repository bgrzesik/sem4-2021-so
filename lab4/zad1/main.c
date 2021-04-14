#include <unistd.h>
#include <sys/wait.h>
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

int child = 0;

void
sigusr1_handler(int sig)
{
    if (!child) {
        WRITE_STATIC(STDOUT_FILENO, "[parent] SIGUSR1\n");
    } else {
        WRITE_STATIC(STDOUT_FILENO, "[child] SIGUSR1\n");
    }
}

int
main(int argc, const char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "./a.out ignore|handler|mask|pending\n");
        return -1;

    } else if (strcmp(argv[1], "ignore") == 0) {
        signal(SIGUSR1, SIG_IGN);

    } else if (strcmp(argv[1], "handler") == 0) {
        signal(SIGUSR1, &sigusr1_handler);

    } else if (strcmp(argv[1], "mask") == 0 || strcmp(argv[1], "pending") == 0) {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGUSR1);

        if (sigprocmask(SIG_BLOCK, &set, NULL) != 0) {
            perror("sigprocmask");
            return -1;
        }
    } else {
        fprintf(stderr, "./a.out ignore|handler|mask|pending\n");
        return -1;
    }

    raise(SIGUSR1);
    
    pid_t child_pid = 0;

    if ((child_pid = fork()) == 0) {
        child = 1;
        raise(SIGUSR1);
    }

    if (strcmp(argv[1], "pending") == 0) {
        sigset_t set;
        sigemptyset(&set);
        
        if (sigpending(&set) != 0) {
            perror("sigpending");
            return -1;
        }

        printf(child ? "[child] " : "[parent] ");

        if (sigismember(&set, SIGUSR1)) {
            printf("SIGUSR1 is pending\n");
        } else {
            printf("SIGUSR1 isn't pending\n");
        }
    }

    if (child) {
        execl(EXEC_FNAME, EXEC_FNAME, argv[1], NULL);
        perror("execl");
        return -1;
    }

    if (!child) {
        int stat, ret = -1;
        do {
           ret = waitpid(child_pid, &stat, 0);
        } while (ret > 0 && ret != child_pid);

        if (WIFEXITED(stat)) {
            printf("[parent] child exited normally\n");
        } else if (WIFSIGNALED(stat)) {
            if (WTERMSIG(stat) == SIGUSR1) {
                printf("[parent] child exited due to SIGUSR1\n");
            } else {
                printf("[parent] child exited due to other signal\n");
            }
        }
    }

    return 0;
}
