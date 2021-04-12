#include <unistd.h>
#include <time.h>
#include <sys/times.h>
#include <sys/wait.h>

#include <stdio.h>
#include <string.h>

#include "vp_list.h"
#include "block_arr.h"
#include "merge.h"

void
vp_list_debug_print(struct vp_list *list)
{
    printf("list.size = %ld\n", list->size); 
    printf("list.capacity = %ld\n", list->capacity); 
    for (size_t i = 0; i < vp_list_size(list); i++) {
        printf("list->array[%ld] = %s\n", i, (char *) vp_list_get(list, i));
    }
}


int 
main(int argc, char **argv)
{
#ifdef LIB_DYNAMIC
    lib_load();
    lib_load_block_arr();
#endif

    struct tms tms_before, tms_after;
    clock_t real_before, real_after;

    real_before = times(&tms_before);

    struct block_arr arr;
    block_arr_init(&arr, 4 * argc);

    for (int i = 1; i < argc; i++) {
        if (fork() == 0) {
            struct block_arr_input input;
            block_arr_input_init(&input);

            char *left = argv[i];
            char *right = strchr(left, ':');

            if (right == NULL) {
                return -1;
            }

            right[0] = 0;

            block_arr_input_add(&input, left, right + 1);

            if (block_arr_add_merged(&arr, &input) != 0) {
                fprintf(stderr, "error: merge failed\n");
                return -1;
            }

            printf("%u: merged %s %s\n", getpid(), left, right + 1);
            right[0] =  ':';

            block_arr_input_free(&input);

            block_arr_free(&arr);
            return 0;
        }
    }

    int ret;
    while(wait(&ret) > 0) {
        if (WEXITSTATUS(ret) != 0) {
            return -1;
        }
    }

    block_arr_free(&arr);

    real_after = times(&tms_after);

    clock_t rtime = real_after - real_before;
    clock_t utime = tms_after.tms_utime - tms_before.tms_utime;
    clock_t stime = tms_after.tms_stime - tms_before.tms_stime;

    float clk_tck = (float) sysconf(_SC_CLK_TCK);

    printf("real time: %4zu %7.3fs \n", rtime, rtime / clk_tck);
    printf("user time: %4zu %7.3fs \n", utime, utime / clk_tck);
    printf("sys  time: %4zu %7.3fs \n\n", stime, stime / clk_tck);


    return 0;
}
