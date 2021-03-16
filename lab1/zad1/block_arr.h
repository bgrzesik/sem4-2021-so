
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

struct block_arr_input {
    struct vp_list files;
};



#ifndef LIB_DYNAMIC


LIB_EXPORT void 
block_arr_init(struct block_arr *arr, size_t size);

LIB_EXPORT size_t 
block_arr_read(struct block_arr *arr, FILE *file);

LIB_EXPORT int 
block_arr_add_merged(struct block_arr *arr, struct block_arr_input *input);

LIB_EXPORT void 
block_arr_remove_block(struct block_arr *arr, size_t block_idx);

LIB_EXPORT void 
block_arr_remove_row(struct block_arr *arr, size_t block_idx, size_t row_idx);

LIB_EXPORT size_t
block_arr_get_block_size(struct block_arr *arr, size_t block_idx);

LIB_EXPORT void 
block_arr_free(struct block_arr *arr);



LIB_EXPORT void
block_arr_input_init(struct block_arr_input *input);

LIB_EXPORT void
block_arr_input_add(struct block_arr_input *input, 
                    const char *left, const char *right);

LIB_EXPORT int
block_arr_input_next(struct block_arr_input *input, 
                     const char **left, const char **right);

LIB_EXPORT void
block_arr_input_free(struct block_arr_input *input);


#else


#define MODULE LIB_MODULE(block_arr)
#define MODULE_EXPORTS(fn)                                                    \
                                                                              \
    fn(void, block_arr_init,                                                  \
            (struct block_arr *arr, size_t size), (arr, size))                \
                                                                              \
    fn(size_t, block_arr_read,                                                \
            (struct block_arr *arr, FILE *file), (arr, file))                 \
                                                                              \
    fn(int, block_arr_add_merged,                                              \
            (struct block_arr *arr, struct block_arr_input *input),           \
            (arr, input))                                                     \
                                                                              \
    fn(void, block_arr_remove_block,                                          \
            (struct block_arr *arr, size_t block_idx), (arr, block_idx))      \
                                                                              \
    fn(void, block_arr_remove_row,                                            \
            (struct block_arr *arr, size_t block_idx, size_t row_idx),        \
            (arr, block_idx, row_idx))                                        \
                                                                              \
    fn(size_t, block_arr_get_block_size,                                      \
            (struct block_arr *arr, size_t block_idx), (arr, block_idx))      \
                                                                              \
    fn(void, block_arr_free,                                                  \
            (struct block_arr *arr), (arr))                                   \
                                                                              \
                                                                              \
    fn(void, block_arr_input_init,                                            \
            (struct block_arr_input *input), (input))                         \
                                                                              \
    fn(void, block_arr_input_add,                                             \
            (struct block_arr_input *input,                                   \
             const char *left, const char *right), (input, left, right))      \
                                                                              \
    fn(void, block_arr_input_next,                                            \
            (struct block_arr_input *input,                                   \
             const char *left, const char *right), (input, left, right))      \
                                                                              \
    fn(void, block_arr_input_free,                                            \
            (struct block_arr_input *input), (input))                         \

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
