
#include <stdlib.h>
#include <stdio.h>

#include "vp_list.h"
#include "lib.h"


#ifndef __BLOCK_ARR__
#define __BLOCK_ARR__


struct block_arr {
    struct vp_list *blocks;
    size_t num;
    size_t cursor;
};

#ifndef LIB_DYNAMIC

LIB_EXPORT void 
block_arr_init(struct block_arr *arr, size_t size);

LIB_EXPORT size_t 
block_arr_read(struct block_arr *arr, FILE *file);

LIB_EXPORT void 
block_arr_remove_block(struct block_arr *arr, size_t block_idx);

LIB_EXPORT void 
block_arr_remove_row(struct block_arr *arr, size_t block_idx, size_t row_idx);

LIB_EXPORT void 
block_arr_free(struct block_arr *arr);

#else


#define MODULE LIB_MODULE(block_arr)
#define MODULE_EXPORTS(fn)                                                           \
                                                                              \
    fn(void, block_arr_init,                                                  \
            (struct block_arr *arr, size_t size), (arr, size))                \
                                                                              \
    fn(size_t, block_arr_read,                                                \
            (struct block_arr *arr, FILE *file), (arr, file))                 \
                                                                              \
    fn(void, block_arr_remove_block,                                          \
            (struct block_arr *arr, size_t block_idx), (arr, block_idx))      \
                                                                              \
    fn(void, block_arr_remove_row,                                            \
            (struct block_arr *arr, size_t block_idx, size_t row_idx),        \
            (arr, block_idx, row_idx))                                        \
                                                                              \
    fn(void, block_arr_free,                                                  \
            (struct block_arr *arr), (arr))                                   \

LIB_TRAMPOLINES(MODULE)

#undef MODULE_EXPORTS
#undef MODULE

#endif


static inline struct vp_list *
block_arr_get(struct block_arr *arr, size_t idx)
{
    return &arr->blocks[idx];
}


#endif
