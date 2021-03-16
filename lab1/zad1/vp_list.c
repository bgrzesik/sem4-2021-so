
#include "vp_list.h"

#include <stdlib.h>
#include <string.h>

void 
vp_list_init(struct vp_list *list)
{
    list->data = NULL;
    list->capacity = 0;
    list->size = 0;
}

static void 
_vp_list_expand(struct vp_list *list)
{
    if (list->data == NULL) {
        list->capacity = VP_LIST_INITIAL_SIZE;
        list->data = calloc(list->capacity, sizeof(list->data[0]));
        return;
    }

    list->capacity <<= 1;
    list->data = realloc(list->data, list->capacity * sizeof(list->data[0]));
}

void 
vp_list_insert(struct vp_list *list, size_t idx, void *block)
{
    if (list->data == NULL) {
        _vp_list_expand(list);
    }

    while (list->capacity < (list->size + 1)) {
        _vp_list_expand(list);
    }

    memmove(&list->data[idx + 1], &list->data[idx], sizeof(void *) * (list->size - idx));

    list->size++;
    list->data[idx] = block;
}

void 
vp_list_append(struct vp_list *list, void *block)
{
    vp_list_insert(list, list->size, block);
}

void 
vp_list_remove(struct vp_list *list, size_t idx)
{
    list->size--;

    for (int i = idx; i < list->size; i++) {
        list->data[i] = list->data[i + 1];
    }

    if (list->size == 0) {
        vp_list_free(list);

    } else if ((list->size << 2) <= list->capacity && 
               list->capacity > VP_LIST_INITIAL_SIZE) {

        list->capacity >>= 1;
        list->data = realloc(list->data, list->size * sizeof(list->data[0]));
    }
}

void 
vp_list_free(struct vp_list *list)
{
    if (list->data != NULL) {
        free(list->data);
        list->data = NULL;
    }

    list->capacity = 0;
    list->size = 0;
}


