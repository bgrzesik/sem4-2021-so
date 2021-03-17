
#include <time.h>
#include <sys/times.h>

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
main(int argc, const char **argv)
{
#ifdef LIB_DYNAMIC
    lib_load();
    lib_load_block_arr();
#endif

    struct block_arr arr;

    block_arr_init(&arr, 10);

    struct block_arr_input input;
    block_arr_input_init(&input);
    block_arr_input_add(&input, "./a100x100k.txt", "./b100x100k.txt");

    block_arr_add_merged(&arr, &input);

    block_arr_input_free(&input);

    printf("block size: %zu\n", block_arr_get_block_size(&arr, 0));

    vp_list_debug_print(block_arr_get(&arr, 0));

    block_arr_remove_block(&arr, 0);
    block_arr_free(&arr);

    return 0;
}
