
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>


int
main(int argc, const char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "./a.out ignore|handler|mask|pending\n");
        return -1;

    }    

    if (strcmp(argv[1], "ignore") == 0 || strcmp(argv[1], "handler") == 0) {
        struct sigaction act;
        if (sigaction(SIGUSR1, NULL, &act) != 0) {
            perror("sigaction");
            return -1;
        }

        if (act.sa_handler == SIG_DFL) {
            printf("[exec] handler = SIG_DFL\n");
        } else if (act.sa_handler == SIG_IGN) {
            printf("[exec] handler = SIG_IGN\n");
        } else {
            printf("[exec] handler = %p\n", act.sa_handler);
        }

    } else if (strcmp(argv[1], "mask") == 0) {
        sigset_t set;
        sigemptyset(&set);

        if (sigprocmask(SIG_UNBLOCK, NULL, &set) != 0) {
            perror("sigprocmask");
            return -1;
        }

        if (sigismember(&set, SIGUSR1)) {
            printf("[exec] SIGUSR1 is blocked\n");
        } else {
            printf("[exec] SIGUSR1 isn't blocked\n");
        }

    } else if (strcmp(argv[1], "pending") == 0) {
        sigset_t set;
        sigemptyset(&set);
        
        if (sigpending(&set) != 0) {
            perror("sigpending");
            return -1;
        }

        if (sigismember(&set, SIGUSR1)) {
            printf("[exec] SIGUSR1 is pending\n");
        } else {
            printf("[exec] SIGUSR1 isn't pending\n");
        }
        
    } else {
        fprintf(stderr, "./a.out ignore|handler|mask|pending\n");
        return -1;
    }

    raise(SIGUSR1);

    return 0;
}
