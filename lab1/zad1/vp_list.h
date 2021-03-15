
#include <stdlib.h>


#define VP_LIST_INITIAL_SIZE 16


struct vp_list {
    void    **array;
    size_t    capacity;
    size_t    size;
};


void 
vp_list_init(struct vp_list *arr);

void 
vp_list_insert(struct vp_list *arr, size_t idx, void *elem);

void 
vp_list_append(struct vp_list *arr, void *elem);

static inline void *
vp_list_get(struct vp_list *arr, size_t idx) 
{
    return arr->array[idx];
}

void 
vp_list_remove(struct vp_list *arr, size_t idx);

void 
vp_list_free(struct vp_list *arr);


