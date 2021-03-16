
#include <stdlib.h>

#include "lib.h"

#ifndef __VP_LIST__
#define __VP_LIST__

#define VP_LIST_INITIAL_SIZE 16


struct vp_list {
    void    **data;
    size_t    capacity;
    size_t    size;
};

#ifndef LIB_DYNAMIC

LIB_EXPORT void 
vp_list_init(struct vp_list *list);

LIB_EXPORT void 
vp_list_insert(struct vp_list *list, size_t idx, void *elem);

LIB_EXPORT void 
vp_list_append(struct vp_list *list, void *elem);

LIB_EXPORT void 
vp_list_remove(struct vp_list *list, size_t idx);

LIB_EXPORT void 
vp_list_free(struct vp_list *list);

#else

#define MODULE LIB_MODULE(vp_list)
#define MODULE_EXPORTS(fn)                                                    \
                                                                              \
    fn(void, vp_list_init,                                                    \
            (struct vp_list *list), (list))                                   \
                                                                              \
    fn(void, vp_list_insert,                                                  \
            (struct vp_list *list, size_t idx, void *elem), (list, idx, elem))\
                                                                              \
    fn(void, vp_list_append,                                                  \
            (struct vp_list *list, void *elem), (list, elem))                 \
                                                                              \
    fn(void, vp_list_remove,                                                  \
            (struct vp_list *list, size_t idx), (list, idx))                  \
                                                                              \
    fn(void, vp_list_free,                                                    \
            (struct vp_list *list), (list))           

LIB_TRAMPOLINES(MODULE)

#undef MODULE_EXPORTS
#undef MODULE

#endif


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
