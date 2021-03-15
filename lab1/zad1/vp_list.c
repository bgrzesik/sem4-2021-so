
#include "vp_list.h"

#include <stdlib.h>

void 
vp_list_init(struct vp_list *arr)
{
    arr->array = NULL;
    arr->capacity = 0;
    arr->size = 0;
}

static void 
_vp_list_expand(struct vp_list *arr)
{
    if (arr->array == NULL) {
        arr->capacity = VP_LIST_INITIAL_SIZE;
        arr->array = calloc(arr->capacity, sizeof(arr->array[0]));
        return;
    }

    arr->capacity <<= 1;
    arr->array = realloc(arr->array, arr->capacity * sizeof(arr->array[0]));
}

void 
vp_list_insert(struct vp_list *arr, size_t idx, void *block)
{
    if (arr->array == NULL) {
        _vp_list_expand(arr);
    }
    while (arr->capacity < (arr->size + 1)) {
        _vp_list_expand(arr);
    }

    for (int i = arr->size; i > idx; i--) {
        arr->array[i] = arr->array[i - 1];
    }

    arr->size++;
    arr->array[idx] = block;
}

void 
vp_list_append(struct vp_list *arr, void *block)
{
    vp_list_insert(arr, arr->size, block);
}

void 
vp_list_remove(struct vp_list *arr, size_t idx)
{
    arr->size--;

    for (int i = idx; i < arr->size; i++) {
        arr->array[i] = arr->array[i + 1];
    }

    if (arr->size == 0) {
        vp_list_free(arr);

    } else if ((arr->size << 2) <= arr->capacity && 
               arr->capacity > VP_LIST_INITIAL_SIZE) {

        arr->capacity >>= 1;
        arr->array = realloc(arr->array, arr->size * sizeof(arr->array[0]));
    }
}

void 
vp_list_free(struct vp_list *arr)
{
    if (arr->array != NULL) {
        free(arr->array);
        arr->array = NULL;
    }

    arr->capacity = 0;
    arr->size = 0;
}


