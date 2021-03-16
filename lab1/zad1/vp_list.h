
#include <stdlib.h>

#ifndef __VP_LIST__
#define __VP_LIST__

#define VP_LIST_INITIAL_SIZE 16


struct vp_list {
    void    **data;
    size_t    capacity;
    size_t    size;
};


void 
vp_list_init(struct vp_list *list);

void 
vp_list_insert(struct vp_list *list, size_t idx, void *elem);

void 
vp_list_append(struct vp_list *list, void *elem);

void 
vp_list_remove(struct vp_list *list, size_t idx);

void 
vp_list_free(struct vp_list *list);


static inline void *
vp_list_get(struct vp_list *list, size_t idx) 
{
    if (idx >= list->size) {
        return NULL;
    }
    return list->data[idx];
}

static inline size_t
vp_list_size(struct vp_list *list)
{
    return list->size;
}


#endif
