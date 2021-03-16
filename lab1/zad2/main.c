#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include <block_arr.h>
#include <merge.h>


struct context {
    int argc;
    char **argv;

    struct block_arr arr;
};

char *
consume(struct context *ctx)
{
    if (ctx->argc <= 0) {
        return NULL;
    }

    ctx->argc--;

    char *ret = *ctx->argv;
    ctx->argv++;

    return ret;
}

size_t 
consume_num(struct context *ctx)
{
    char *arg = consume(ctx);
    
    if (arg == NULL) {
        fprintf(stderr, "error: no argument passed\n");
        return -1;
    }
    
    char *arg_end = NULL;
    size_t val = strtoull(arg, &arg_end, 10);
    if (*arg_end != '\0') {
        fprintf(stderr, "error: invalid argument passed\n");
        return -1;
    }

    return val;
}

void 
retreat(struct context *ctx)
{
    ctx->argc++;
    ctx->argv--;
}
 
int
cmd_create_table(struct context *ctx) {
    size_t size = consume_num(ctx);
    if (size == -1) {
        return -1;
    }

    block_arr_init(&ctx->arr, size);

    return 0;
}

int
cmd_merge_files(struct context *ctx)
{
    char *left = consume(ctx);
    char *right = NULL; 

    if (left != NULL) {
        right = strchr(left, ':');
    }

    do {
        if (right == NULL) {
            fprintf(stderr, "error: at least one pair has to given\n");
            return -1;
        }

        right[0] = '\0';
        right++;

        FILE *fleft = fopen(left, "r");
        FILE *fright = fopen(right, "r");

        if (fleft != NULL && fright != NULL) {
            FILE *tmp = merge_files(fleft, fright);

            if (tmp != NULL) {
                block_arr_read(&ctx->arr, tmp);
                fclose(tmp);
            }
        } else {
            fprintf(stderr, "error: at least one of the files is missing\n");
        }

        if (fleft != NULL) {
            fclose(fleft);
        }
        if (fright != NULL) {
            fclose(fright);
        }


        left = consume(ctx);

        if (left != NULL) {
            right = strchr(left, ':');
        } else {
            right = NULL;
        }

    } while (right != NULL);

    retreat(ctx);

    return 0;
}

int
cmd_print_block(struct context *ctx)
{
    size_t idx = consume_num(ctx);
    if (idx == -1) {
        return -1;
    }
    
    struct vp_list *list = block_arr_get(&ctx->arr, idx);
    for (size_t i = 0; i < vp_list_size(list); i++) {
        puts((char *) vp_list_get(list, i));
    }

    return 0;
}

int
cmd_remove_block(struct context *ctx)
{
    size_t idx = consume_num(ctx);
    if (idx == -1) {
        return -1;
    }

    block_arr_remove_block(&ctx->arr, idx);
    return 0;
}

int
cmd_remove_row(struct context *ctx)
{
    size_t block_idx = consume_num(ctx);
    if (block_idx == -1) {
        return -1;
    }

    size_t row_idx = consume_num(ctx);
    if (row_idx == -1) {
        return -1;
    }

    block_arr_remove_row(&ctx->arr, block_idx, row_idx);
    return 0;
}

struct _cmd_t {
    const char *name;
    int (*func)(struct context *);
};

const struct _cmd_t cmds[] = {
    { "create_table", &cmd_create_table },
    { "merge_files", &cmd_merge_files },
    { "print_block", &cmd_print_block },
    { "remove_block", &cmd_remove_block },
    { "remove_row", &cmd_remove_row },
};

int 
main(int argc, const char **argv) 
{
    struct context ctx;
    ctx.argc = argc - 1;
    ctx.argv = (char **) argv + 1;

    const char *cmd = consume(&ctx);

    while (cmd != NULL) {
        const size_t ncmds = sizeof(cmds) / sizeof(cmds[0]);

        int i;
        for (i = 0; i < ncmds; i++) {
            if (strcmp(cmd, cmds[i].name) == 0) {
                printf("cmd: %s\n", cmds[i].name);

                int ret = cmds[i].func(&ctx);
                
                if (ret != 0) {
                    return ret;
                }

                break;
            }
        }

        if (i == ncmds) {
            fprintf(stderr, "error: unknown command: %s\n", cmd);
            return -1;
        }

        cmd = consume(&ctx);
    }

    block_arr_free(&ctx.arr);

    return 0;
}
