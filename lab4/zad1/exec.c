
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

    } else if (strcmp(argv[1], "ignore") == 0) {
    } else if (strcmp(argv[1], "handler") == 0) {
    } else if (strcmp(argv[1], "mask") == 0) {
    } else if (strcmp(argv[1], "pending") == 0) {
    } else {
        fprintf(stderr, "./a.out ignore|handler|mask|pending\n");
        return -1;
    }

    raise(SIGUSR1);

    if (strcmp(argv[1], "pending") == 0) {
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
    }

    return 0;
}
