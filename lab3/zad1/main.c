

#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

int
main(int argc, const char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "a.out: <child count>\n");
        return -1;
    }

    unsigned long n = strtoul(argv[1], NULL, 10);

    if (n == 0) {
        return -1;
    }

    for (int i = 0; i < n; i++) {
        if (fork() == 0) {
            printf("Child no. %d with pid %u\n", i, getpid());
            return 0;
        }
    }

    while(wait(NULL) > 0);

    return 0;
}


