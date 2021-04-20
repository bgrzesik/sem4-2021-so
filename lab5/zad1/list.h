
#include <stdlib.h>

#ifndef __VP_LIST__
#define __VP_LIST__

#define VP_LIST_INITIAL_SIZE 16

#define LIST_FOREACH(list, type, iter)                   \
    for (type * iter = (type *) list_begin(list);        \
         iter < (type *) list_end(list);                 \
         (iter)++)

#define LIST_FOREACH_REV(list, type, iter)               \
    for (type * iter = (type *) list_last(list);         \
         iter >= (type *) list_begin(list);               \
         (iter)--)

#define LIST_EMBRACE(list, type, value)                  \
    *((type *) list_append(list)) = value;


struct list {
    char     *data;
    size_t    capacity;
    size_t    size;
    size_t    stride;
};


void 
list_init(struct list *list, size_t stride);

void *
list_insert(struct list *list, size_t idx);

void
list_remove(struct list *list, size_t idx);

void 
list_free(struct list *list);

static inline void *
list_append(struct list *list)
{
    return list_insert(list, list->size);
}

static inline void *
list_get(struct list *list, size_t idx) 
{
    if (idx >= list->size) {
        return NULL;
    }

    return (void *) &list->data[idx * list->stride];
}

static inline size_t
list_size(struct list *list)
{
    return list->size;
}

static inline void
list_pop(struct list *list)
{
    list_remove(list, list->size - 1);
}

static inline void *
list_last(struct list *list)
{
    if (list_size(list) == 0) {
        return NULL;
    }

    return list_get(list, list_size(list) - 1);
}

static inline void *
list_begin(struct list *list)
{
    return list->data;
}

static inline void *
list_end(struct list *list)
{
    return &list->data[list->size * list->stride];
}



#endif
