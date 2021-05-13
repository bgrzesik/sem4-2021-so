

#include <stdlib.h>
#include <string.h>
#include "sems.h"


#ifndef __RINGBUF_H__
#define __RINGBUF_H__

/*
    A blocking ring fifo queue.
    Implementation based on Stallings book.
 */

struct ringbuf {
    size_t stride;
    size_t cap;
    char *data;

    size_t start;
    size_t end;

    semaphore_t lock;
    semaphore_t free_slots;
    semaphore_t taken_slots;
};


static inline int
ringbuf_init(struct ringbuf *buf, size_t stride, size_t cap, void *data)
{
    buf->stride = stride;
    buf->cap = cap;
    buf->data = (char *) data;

    buf->start = -1;
    buf->end = 0;

    int ret = 0;
    ret |= semaphore_init(&buf->lock, 1);
    ret |= semaphore_init(&buf->free_slots, buf->cap);
    ret |= semaphore_init(&buf->taken_slots, 0);

    if (ret != 0) {
        return ret;
    }

    return 0;
}

static inline size_t
ringbuf_size(struct ringbuf *buf) 
{
    if (buf->start == -1) {
        return 0;

    } else if (buf->start <= buf->end) {
        return buf->end - buf->start + 1;

    } else {
        return buf->cap - buf->start + buf->end;
    }
}

static inline size_t
ringbuf_put(struct ringbuf *buf, void *elem)
{
    if (semaphore_wait(&buf->free_slots) == -1) {
        return -1;
    }

    if (semaphore_wait(&buf->lock) == -1) {
        return -1;
    }

    if (buf->start == -1) {
        buf->start = 0;
        buf->end = 0;
    } else {
        buf->end = (buf->end + 1) % buf->cap;
    }

    memcpy(&buf->data[buf->end * buf->stride], elem, buf->stride);

    size_t size = ringbuf_size(buf);
    
    if (semaphore_signal(&buf->lock) == -1) {
        return -1;
    }

    if (semaphore_signal(&buf->taken_slots) == -1) {
        return -1;
    }

    return size;
}

static inline size_t
ringbuf_get(struct ringbuf *buf, void *elem)
{
    if (semaphore_wait(&buf->taken_slots) == -1) {
        return -1;
    }

    if (semaphore_wait(&buf->lock) == -1) {
        return -1;
    }


    memcpy(elem, &buf->data[buf->start * buf->stride], buf->stride);

    if (buf->start == buf->end) {
        buf->start = -1;
    } else {
        buf->start = (buf->start + 1) % buf->cap;
    }

    size_t size = ringbuf_size(buf);
    
    if (semaphore_signal(&buf->lock) == -1) {
        return -1;
    }

    if (semaphore_signal(&buf->free_slots) == -1) {
        return -1;
    }

    return size;
}

static inline int
ringbuf_free(struct ringbuf *buf)
{
    int ret = 0;
    ret |= semaphore_close(&buf->lock);
    ret |= semaphore_close(&buf->taken_slots);
    ret |= semaphore_close(&buf->free_slots);

    if (ret != 0) {
        return ret;
    }

    return 0;
}


#endif
