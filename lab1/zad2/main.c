#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>
#include <unistd.h>

#include <block_arr.h>
#include <merge.h>


struct context {
    int argc;
    char **argv;

    int arr_init;
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

    if (ctx->arr_init) {
        block_arr_free(&ctx->arr);
    }
    block_arr_init(&ctx->arr, size);
    ctx->arr_init = 1;

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

    struct block_arr_input input;
    block_arr_input_init(&input);

    do {
        if (right == NULL) {
            fprintf(stderr, "error: at least one pair has to given\n");
            return -1;
        }

        right[0] = '\0';
        right++;

        block_arr_input_add(&input, left, right);

        right--;
        right[0] = ':';

        left = consume(ctx);

        if (left != NULL) {
            right = strchr(left, ':');
        } else {
            right = NULL;
        }

    } while (right != NULL);

    retreat(ctx);

    block_arr_add_merged(&ctx->arr, &input);

    block_arr_input_free(&input);

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
cmd_block_size(struct context *ctx)
{
    size_t idx = consume_num(ctx);
    if (idx == -1) {
        return -1;
    }

    size_t size = block_arr_get_block_size(&ctx->arr, idx);
    printf("size: %zu\n", size);

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

struct cmd_info {
    const char *name;
    int (*func)(struct context *);
};

const struct cmd_info cmd_infos[] = {
    { "create_table", &cmd_create_table },
    { "merge_files", &cmd_merge_files },
    { "print_block", &cmd_print_block },
    { "block_size", &cmd_block_size },
    { "remove_block", &cmd_remove_block },
    { "remove_row", &cmd_remove_row },
};

int 
main(int argc, const char **argv) 
{

#ifdef LIB_DYNAMIC
    lib_load();
    lib_load_block_arr();
#endif

    struct context ctx;
    ctx.arr_init = 0;
    ctx.argc = argc - 1;
    ctx.argv = (char **) argv + 1;


    char **last_argv = ctx.argv;
    const char *cmd = consume(&ctx);

    while (cmd != NULL) {
        const size_t ncmds = sizeof(cmd_infos) / sizeof(cmd_infos[0]);

        const struct cmd_info *info = NULL;

        for (int i = 0; i < ncmds; i++) {
            if (strcmp(cmd, cmd_infos[i].name) == 0) {
                info = &cmd_infos[i];
                break;
            }
        }

        if (info != NULL) {
            struct tms tms_before, tms_after;
            clock_t real_before, real_after;

            real_before = times(&tms_before);
            int ret = (*(info->func))(&ctx);
            real_after = times(&tms_after);

            fputs("cmd: ", stdout);
            while (last_argv != ctx.argv) {
                fputs(*last_argv, stdout);
                fputs(" ", stdout);

                last_argv++;
            }
            fputs("\n", stdout);
            
            if (ret != 0) {
                if (ctx.arr_init) {
                    block_arr_free(&ctx.arr);
                }

                return ret;
            }

            clock_t rtime = real_after - real_before;
            clock_t utime = tms_after.tms_utime - tms_before.tms_utime;
            clock_t stime = tms_after.tms_stime - tms_before.tms_stime;

            float clk_tck = (float) sysconf(_SC_CLK_TCK);

            printf("real time: %4zu %7.3fs \n", rtime, rtime / clk_tck);
            printf("user time: %4zu %7.3fs \n", utime, utime / clk_tck);
            printf("sys  time: %4zu %7.3fs \n\n", stime, stime / clk_tck);

        } else {
            fprintf(stderr, "error: unknown command: %s\n", cmd);

            if (ctx.arr_init) {
                block_arr_free(&ctx.arr);
            }

            return -1;
        }

        cmd = consume(&ctx);
    }

    if (ctx.arr_init) {
        block_arr_free(&ctx.arr);
    }

    return 0;
}
