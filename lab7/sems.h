
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

#if defined(USE_POSIX)
/* shmget, shmclt, shmat, shmdt */
#include <sys/shm.h> 
#include <sys/ipc.h> 
#include <semaphore.h>

#elif defined(USE_SYSV)
/* shm_open, shm_close, shm_unlink, mmap, munmap */
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/ipc.h>

#else
#error "Defined either USE_POSIX or USE_SYSV"

#endif



#ifndef __SEMS_H__
#define __SEMS_H__
/* semaphores wrapper */

#ifdef USE_POSIX
#define semaphore_t sem_t
#else /* USE_SYSV */
#define semaphore_t int
#endif


static inline int
semaphore_init(semaphore_t *sem, int val)
{
#ifdef USE_POSIX
    if (sem_init(sem, 1, val) == -1) {
        perror("sem_init");
        return -1;
    }
#else /* USE_SYSV */
    if ((*sem = semget(IPC_PRIVATE, 1, 0666)) == -1) {
        perror("semget");
        return -1;
    }
    
    if (semctl(*sem, 0, SETVAL, val) == -1) {
        perror("semctl");
        return -1;
    }
#endif

    return 0;
}

static inline int 
semaphore_wait(semaphore_t *sem)
{
#ifdef USE_POSIX
    if (sem_wait(sem) == -1) {
        perror("sem_wait");
        return -1;
    }

#else /* USE_SYSV */
    struct sembuf sembuf;
    sembuf.sem_num = 0;
    sembuf.sem_op = -1;
    sembuf.sem_flg = SEM_UNDO;

    if (semop(*sem, &sembuf, 1) == -1) {
        perror("semop");
        return -1;
    }
#endif

    return 0;
}

static inline int
semaphore_signal(semaphore_t *sem)
{
#ifdef USE_POSIX
    if (sem_post(sem) == -1) {
        perror("sem_post");
        return -1;
    }

#else /* USE_SYSV */
    struct sembuf sembuf;
    sembuf.sem_num = 0;
    sembuf.sem_op = 1;
    sembuf.sem_flg = SEM_UNDO;

    if (semop(*sem, &sembuf, 1) == -1) {
        perror("semop");
        return -1;
    }
#endif

    return 0;
}

static inline int 
semaphore_close(semaphore_t *sem)
{
#ifdef USE_POSIX
    if (sem_destroy(sem) == -1) {
        perror("sem_destroy");
        return -1;
    }

#else /* USE_SYSV */
    if (*sem != 0 && semctl(*sem, 0, IPC_RMID) != 0) {
        perror("semctl");
        return -1;
    }

    *sem = 0;
#endif

    return 0;
}


#endif

