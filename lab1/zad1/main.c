
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

#if 1
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
    

#elif 0
    struct vp_list arr;

    vp_list_init(&arr);
    
    vp_list_append(&arr, "<block 00>");
    vp_list_append(&arr, "<block 01>");
    vp_list_append(&arr, "<block 02>");
    vp_list_append(&arr, "<block 03>");
    vp_list_append(&arr, "<block 04>");
    vp_list_append(&arr, "<block 05>");
    vp_list_append(&arr, "<block 06>");
    vp_list_append(&arr, "<block 07>");
    vp_list_append(&arr, "<block 08>");
    vp_list_append(&arr, "<block 09>");
    vp_list_append(&arr, "<block 10>");
    vp_list_append(&arr, "<block 11>");
    vp_list_append(&arr, "<block 12>");
    vp_list_append(&arr, "<block 13>");
    vp_list_append(&arr, "<block 14>");
    vp_list_append(&arr, "<block 15>");
    vp_list_append(&arr, "<block 16>");
    vp_list_append(&arr, "<block 17>");
    vp_list_append(&arr, "<block 18>");
    vp_list_append(&arr, "<block 19>");
    vp_list_append(&arr, "<block 20>");
    vp_list_debug_print(&arr);

    vp_list_insert(&arr, 10, "<block (10)>");
    vp_list_insert(&arr, 0, "<block (0)>");
    vp_list_debug_print(&arr);

    vp_list_remove(&arr, 11);
    vp_list_remove(&arr, 0);

    vp_list_debug_print(&arr);

    while (arr.size != 0) {
        vp_list_remove(&arr, 0);
        vp_list_debug_print(&arr);
    }
    
    vp_list_debug_print(&arr);

    vp_list_free(&arr);
#endif

    return 0;
}
