
#include "block_arr.h"
#include "merge.h"

#include <stdio.h>
#include <string.h>

LIB_EXPORT void 
block_arr_init(struct block_arr *arr, size_t num)
{
    arr->blocks = calloc(num, sizeof(struct vp_list));
    arr->num = num;
    arr->cursor = 0;

    for (size_t i = 0; i < num; i++) {
        vp_list_init(&arr->blocks[i]);
    }
}

LIB_EXPORT size_t 
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

    if (line != NULL) {
        free(line);
    }

    return arr->cursor++;
}

LIB_EXPORT int 
block_arr_add_merged(struct block_arr *arr, struct block_arr_input *input)
{
    const char *left  = NULL;
    const char *right = NULL;

    while (block_arr_input_next(input, &left, &right)) {
        if (left == NULL || right == NULL) {
            return -1;
        }

        FILE *fleft = fopen(left, "r");
        FILE *fright = fopen(right, "r");

        if (fleft != NULL && fright != NULL) {
            FILE *tmp = merge_files(fleft, fright);

            if (tmp != NULL) {
                block_arr_read(arr, tmp);
                fclose(tmp);
            }
        } else {
            if (fleft != NULL) {
                fclose(fleft);
            }
            if (fright != NULL) {
                fclose(fright);
            }

            return -1;
        }

        fclose(fleft);
        fclose(fright);
    }

    return 0;
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

LIB_EXPORT void 
block_arr_remove_block(struct block_arr *arr, size_t block_idx)
{
    arr->cursor--;
    _block_arr_free_block(arr, block_idx);

    memmove(&arr->blocks[block_idx], &arr->blocks[block_idx + 1], 
            (arr->cursor - block_idx) * sizeof(struct vp_list));

    vp_list_init(&arr->blocks[arr->cursor]);
}

LIB_EXPORT void 
block_arr_remove_row(struct block_arr *arr, size_t block_idx, size_t row_idx)
{
    struct vp_list *list = &arr->blocks[block_idx];
    char *row = (char*) vp_list_remove(list, row_idx);
    free(row);
}

LIB_EXPORT size_t
block_arr_get_block_size(struct block_arr *arr, size_t block_idx)
{
    struct vp_list *list = &arr->blocks[block_idx];
    return vp_list_size(list);
}

LIB_EXPORT void 
block_arr_free(struct block_arr *arr)
{

    for (size_t i = 0; i < arr->cursor; i++) {
        _block_arr_free_block(arr, i);
    }

    if (arr->blocks != NULL) {
        free(arr->blocks);
        arr->blocks = NULL;
    }
}

LIB_EXPORT void
block_arr_input_init(struct block_arr_input *input)
{
    vp_list_init(&input->files);
}

LIB_EXPORT void
block_arr_input_add(struct block_arr_input *input, 
                    const char *left, const char *right)
{
    vp_list_append(&input->files, (void *) strdup(left));
    vp_list_append(&input->files, (void *) strdup(right));
}

LIB_EXPORT int
block_arr_input_next(struct block_arr_input *input, 
                     const char **left, const char **right)
{
    if (*left == NULL && *right == NULL) {
        vp_list_reverse(&input->files);
    } else {
        free((void *) *left);
        free((void *) *right);
    }

    if (vp_list_size(&input->files) == 0) {
        return 0;
    }

    *left = vp_list_pop(&input->files);
    *right = vp_list_pop(&input->files);

    return 1;
}

LIB_EXPORT void
block_arr_input_free(struct block_arr_input *input)
{
    vp_list_free(&input->files);
}
