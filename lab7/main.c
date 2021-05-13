
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include "sems.h"
#include "ringbuf.h"


#ifndef OVEN_CAP
#define OVEN_CAP 5
#endif

#ifndef TABLE_CAP
#define TABLE_CAP 5
#endif

#ifndef COOK_COUNT
#define COOK_COUNT 20
#endif


#ifndef COURIER_COUNT
#define COURIER_COUNT 20
#endif

struct shared_mem {
    int oven_data[OVEN_CAP];
    struct ringbuf oven;

    int table_data[TABLE_CAP];
    struct ringbuf table;
};


static int
cook_process(struct shared_mem *mem)
{
    while (1) {
        int pizza = rand() % 10;
        size_t ret;
        
        printf("(%d %ld) preparing pizza %d\n", 
               getpid(), (unsigned long) time(NULL), pizza);

        sleep(1 + (rand() & 1));
        ret = ringbuf_put(&mem->oven, &pizza);

        printf("(%d %ld) put pizza %d to oven (%ld/%d)\n",
               getpid(), (unsigned long) time(NULL), pizza,
               ret, OVEN_CAP);

        sleep(4 + (rand() & 1));
        ret = ringbuf_get(&mem->oven, &pizza);

        printf("(%d %ld) taken pizza %d from oven (%ld/%d) and putting on table (%ld/%d)\n", 
               getpid(), (unsigned long) time(NULL), pizza, 
               ret, OVEN_CAP, ringbuf_size(&mem->table), TABLE_CAP);

        /* TODO in oven & on table */

        ret = ringbuf_put(&mem->table, &pizza);
    }
    return 0;
}

static int
courier_process(struct shared_mem *mem)
{
    while (1) {
        int pizza;
        size_t ret;

        ret = ringbuf_get(&mem->table, &pizza);

        printf("(%d %ld) got pizza %d from table (%ld/%d)\n", 
               getpid(), (unsigned long) time(NULL), pizza,
               ret, OVEN_CAP);

        /* TODO on table */

        sleep(4 + (rand() & 1));

        printf("(%d %ld) delivered pizza %d\n", 
               getpid(), (unsigned long) time(NULL), pizza);

        sleep(4 + (rand() & 1));
    }
    return 0;
}


int
main(int argc, const char *argv[])
{
    srand(time(NULL));
    
    struct shared_mem *mem;
#ifdef USE_SYSV
    mem = (struct shared_mem *) mmap(NULL, sizeof(struct shared_mem), 
                              PROT_READ | PROT_WRITE, 
                              MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (mem == (void *) -1) {
        perror("mmap");
        return -1;
    }
#else
    int shmid = shmget(IPC_PRIVATE, 
                       sizeof(struct shared_mem), 
                       0666 | IPC_CREAT | IPC_EXCL);
    if (shmid == -1) {
        perror("shmget");
        return -1;
    }

    mem = (struct shared_mem *) shmat(shmid, NULL, 0);
    if (mem == (void *) -1) {
        perror("shmat");
        return -1;
    }
#endif

    ringbuf_init(&mem->oven, 
                 sizeof(mem->oven_data[0]), 
                 sizeof(mem->oven_data) / sizeof(mem->oven_data[0]), 
                 mem->oven_data);

    ringbuf_init(&mem->table, 
                 sizeof(mem->table_data[0]), 
                 sizeof(mem->table_data) / sizeof(mem->table_data[0]), 
                 mem->table_data);

    for (int i = 0; i < COOK_COUNT; i++) {
        if (fork() == 0) {
            return cook_process(mem);
        }
    }

    for (int i = 0; i < COURIER_COUNT; i++) {
        if (fork() == 0) {
            return courier_process(mem);
        }
    }

    while (wait(NULL) > 0);

    ringbuf_free(&mem->oven);
    ringbuf_free(&mem->table);

#ifdef USE_SYSV
    if (munmap(mem, sizeof(struct shared_mem)) == -1) {
        perror("munmap");
    }
#else
    if (shmdt(mem) == -1) {
        perror("shmdt");
    }

    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
    }
#endif

    return 0;
}

