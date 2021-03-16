
#include <stdlib.h>
#include <stdio.h>

#include "vp_list.h"


#ifndef __BLOCK_ARR__
#define __BLOCK_ARR__


struct block_arr {
    struct vp_list *blocks;
    size_t num;
    size_t cursor;
};


void 
block_arr_init(struct block_arr *arr, size_t size);

size_t 
block_arr_read(struct block_arr *arr, FILE *file);

void 
block_arr_remove_block(struct block_arr *arr, size_t block_idx);

void 
block_arr_remove_row(struct block_arr *arr, size_t block_idx, size_t row_idx);

void 
block_arr_free(struct block_arr *arr);

#endif
