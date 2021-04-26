
#ifndef __COMMON_H__
#define __COMMON_H__


#define FIFO_NAME "./fifo"
#define FIFO_READ_LOCK "./fifo.read.lock"
#define FIFO_WRITE_LOCK "./fifo.write.lock"


struct fifo {
    int fd;
    int lock;
};

struct packet {
    size_t line_no;
    char data[];
};

int
open_fifo(struct fifo *fifo, const char *pathname, int flags);

int
close_fifo(struct fifo *fifo);

int 
read_fifo(struct fifo *fifo, void *buf, size_t size);

int 
write_fifo(struct fifo *fifo, const void *buf, size_t size);


#endif
