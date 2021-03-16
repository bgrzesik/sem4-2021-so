
#include "block_arr.h"
#include <stdio.h>
#include <string.h>

void 
block_arr_init(struct block_arr *arr, size_t num)
{
    arr->blocks = calloc(num, sizeof(struct block_arr));
    arr->num = num;
    arr->cursor = 0;

    for (size_t i = 0; i < num; i++) {
        vp_list_init(&arr->blocks[i]);
    }
}

size_t 
block_arr_read(struct block_arr *arr, FILE *file)
{
    if (file == NULL || arr->cursor >= arr->num) {
        return -1;
    }

    struct vp_list *list = &arr->blocks[arr->cursor];

    char *line = NULL;
    size_t nread, len = 0;

    while ((nread = getline(&line, &len, file)) !=  -1) {
        line[nread - 1] = '\0';
        vp_list_append(list, strdup(line));
        line[nread - 1] = '\n';
    }

    arr->cursor++;
}

static void
_block_arr_free_block(struct block_arr *arr, size_t block_idx)
{
    struct vp_list *list = &arr->blocks[block_idx];

    for (size_t i = 0; i < vp_list_size(list); i++) {
        free(vp_list_get(list, i));
    }

    vp_list_free(list);
}

void 
block_arr_remove_block(struct block_arr *arr, size_t block_idx)
{
    arr->cursor--;
    _block_arr_free_block(arr, block_idx);
    memmove(&arr->blocks[block_idx + 1], &arr->blocks[block_idx], 
            (arr->cursor - block_idx) * sizeof(struct block_arr));
}

void 
block_arr_remove_row(struct block_arr *arr, size_t block_idx, size_t row_idx)
{
    struct vp_list *list = &arr->blocks[block_idx];
    vp_list_remove(list, row_idx);
}

void 
block_arr_free(struct block_arr *arr)
{

    for (size_t i = 0; i < arr->num; i++) {
        _block_arr_free_block(arr, i);
    }

    if (arr->blocks != NULL) {
        free(arr->blocks);
        arr->blocks = NULL;
    }
}
