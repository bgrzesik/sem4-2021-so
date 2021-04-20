#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "list.h"
#include "interpreter.h"


int
main(int argc, const char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: <file>|-\n");
        return -1;
    }

    FILE *fin;

    if (argv[1][0] == '-' && argv[1][1] == '\0') {
        fin = stdin;
    } else {
        fin = fopen(argv[1], "r");
    }

    if (fin == NULL) {
        fprintf(stderr, "error: unable to read file: %s\n", argv[1]);
        return -1;
    }
    

    if (eval(fin) != 0) {
        return -1;
    }
    
    
    fclose(fin);

    
    return 0;
}
