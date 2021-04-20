
#include "list.h"

#include <stdlib.h>
#include <string.h>

void 
list_init(struct list *list, size_t stride)
{
    list->data = NULL;
    list->capacity = 0;
    list->size = 0;
    list->stride = stride;
}

static void 
_list_expand(struct list *list)
{
    if (list->data == NULL) {
        list->capacity = VP_LIST_INITIAL_SIZE;
        list->data = calloc(list->capacity, list->stride);
        return;
    }

    list->capacity <<= 1;
    list->data = realloc(list->data, list->stride * list->capacity);
}

void *
list_insert(struct list *list, size_t idx)
{
    if (list->data == NULL) {
        _list_expand(list);
    }
    
    while (list->capacity < (list->size + 1)) {
        _list_expand(list);
    }

    memmove(&list->data[(idx + 1) * list->stride], 
            &list->data[idx       * list->stride], 
            list->stride * (list->size - idx));

    list->size++;
    return (void *) &list->data[idx * list->stride];
}

void
list_remove(struct list *list, size_t idx)
{
    if (list->size == 0) {
        return;
    }

    list->size--;

    if (list->size == 0) {
        list_free(list);
        return;
    } 

    memmove(&list->data[(idx + 1) * list->stride], 
            &list->data[idx       * list->stride], 
            list->stride * (list->size - idx));

    if ((list->size << 2) <= list->capacity && 
               list->capacity > VP_LIST_INITIAL_SIZE) {

        list->capacity >>= 1;
        list->data = realloc(list->data, list->size * list->stride);
    }
}

void 
list_free(struct list *list)
{
    if (list->data != NULL) {
        free(list->data);
        list->data = NULL;
    }

    list->capacity = 0;
    list->size = 0;
}
