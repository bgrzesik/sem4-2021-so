#include <stdio.h>
#include <stdlib.h>
#include <string.h>


const char *
consume(int *argc, const char ***argv)
{
    if (*argc <= 0) {
        return NULL;
    }

    (*argc)--;
    const char *ret = *argv[0];

    *argv = &(*argv)[1];

    return ret;
}

int
cmd_create_table(int *argc, const char ***argv) {
    const char *arg = consume(argc, argv);
    
    if (arg == NULL) {
        return -1;
    }

    size_t size = strtoull(arg, NULL, 10);

    printf("create_table %ld \n", size);

    return 0;
}


struct _cmd_t
{
    const char *name;
    int (*func)(int *, const char ***);
};

const struct _cmd_t cmds[] = {
    { "create_table", &cmd_create_table }
};

int 
main(int argc, const char **argv) 
{
    const char *cmd = consume(&argc, &argv);
    while (cmd != NULL) {

        for (int i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
            if (strcmp(cmd, cmds[i].name) == 0) {
                int ret = cmds[i].func(&argc, &argv);
                
                if (ret != 0) {
                    return ret;
                }
            }
        }



        cmd = consume(&argc, &argv);
    }


    return 0;
}
