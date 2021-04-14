#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>


#ifndef __COMMON_H__
#define __COMMON_H__


#ifdef CATCHER
#define TAG "[catcher]"
#endif

#ifdef SENDER
#define TAG "[sender]"
#endif

#ifndef TAG
#error "SENDER or CATCHER must be defined"
#endif



#define WRITE_STATIC(fd, text)                  \
    do {                                        \
        static const char msg[] = (text);       \
        write((fd), msg, sizeof(msg));          \
    } while (0)


void
write_dec(int fd, size_t pp);

int
method_kill(int sig, pid_t pid, int n);

int
method_sigqueue(int sig, pid_t pid, int n);

int
signal_block_all(void);

int
transmit(int (*method)(int, pid_t, int), 
         int msg_signal, int end_signal, 
         pid_t catcher, int n);

int
receive(void);

#endif
