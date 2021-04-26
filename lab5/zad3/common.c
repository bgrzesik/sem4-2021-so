
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>


#include "common.h"


int
open_fifo(struct fifo *fifo, const char *pathname, int flags)
{
    if (access(pathname, R_OK | W_OK) != 0) {
        if (mkfifo(FIFO_NAME, 0666) == -1) {
            perror("mkfifo");
            return -1;
        }
    }

    fifo->fd = open(pathname, flags);
    if (fifo->fd == -1) {
        perror("open");
        return -1;
    }

    if (strlen(pathname) + sizeof(".write.lock") >= PATH_MAX) {
        return -1;
    }

    char lock_name[PATH_MAX];
    lock_name[0] = 0;
    strcat(lock_name, pathname);
    if (flags == O_RDONLY) {
        strcat(lock_name, ".read.lock");
    } else {
        strcat(lock_name, ".write.lock");
    }

    static const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
    fifo->lock = open(lock_name, O_WRONLY | O_CREAT | O_TRUNC, mode);

    if (fifo->lock == -1) {
        return -1;
    }

    return 0;
}

int
close_fifo(struct fifo *fifo)
{
    close(fifo->fd);
    close(fifo->lock);

    fifo->fd = 0;
    fifo->lock = 0;

    return 0;
}

int 
read_fifo(struct fifo *fifo, void *buf, size_t size)
{
    if (flock(fifo->lock, LOCK_EX) != 0) {
        perror("fflock");
        return -1;
    }

    int bytes = read(fifo->fd, buf, size);
    if (bytes == -1) {
        perror("read");
        return -1;
    }

    if (flock(fifo->lock, LOCK_UN) != 0) {
        perror("fflock");
        return -1;
    }

    return bytes;
}

int 
write_fifo(struct fifo *fifo, const void *buf, size_t size)
{
    if (flock(fifo->lock, LOCK_EX) != 0) {
        perror("fflock");
        return -1;
    }

    int bytes = write(fifo->fd, buf, size);
    if (bytes == -1) {
        perror("write");
        return -1;
    }

    if (flock(fifo->lock, LOCK_UN) != 0) {
        perror("fflock");
        return -1;
    }

    return bytes;
}

