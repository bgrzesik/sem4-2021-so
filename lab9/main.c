#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

/*
 Solution to blocking pthread_cond_wait was taken from:
     https://stackoverflow.com/a/64441638/13252719
     pthread_cleanup_push((void (*)(void *))pthread_mutex_unlock, &mutex);
 */

#define SANTA_HELPS_ELVES 3
#define SANTA_ITERATIONS 3
#define ELF_COUNT 10
#define REINDEER_COUNT 9
#define TAG_SANTA    "Santa        "
#define TAG_ELF      "Elf %zu        "
#define TAG_REINDEER "Reindeer %zu   "

struct north_pole {
    pthread_mutex_t mutex;
    pthread_cond_t  wake_santa;

    pthread_cond_t  elf_queue;
    pthread_cond_t  santa_helping;

    pthread_cond_t  reindeers_waiting;
    pthread_cond_t  reindeers_delivering;

    size_t helpless_elves_count;
    size_t helpless_elves[SANTA_HELPS_ELVES];

    size_t returned_reindeers;

    int ongoing_delivery;
} north_pole = {
        .mutex = PTHREAD_MUTEX_INITIALIZER,

        .wake_santa = PTHREAD_COND_INITIALIZER,
        .elf_queue = PTHREAD_COND_INITIALIZER,
        .santa_helping = PTHREAD_COND_INITIALIZER,
        .reindeers_waiting = PTHREAD_COND_INITIALIZER,
        .reindeers_delivering = PTHREAD_COND_INITIALIZER,

        .helpless_elves_count = 0,
        .helpless_elves = {0 /* entire array is zeroed anyway */},

        .returned_reindeers = 0,

        .ongoing_delivery = 0,
};

struct threads {
    pthread_t santa;
    union {
        struct {
            pthread_t elves[ELF_COUNT];
            pthread_t reindeer[REINDEER_COUNT];
        };

        pthread_t elves_and_reindeers[ELF_COUNT + REINDEER_COUNT];
    };
} threads;

static size_t
rand_num(void);

static FILE *rand_fd = NULL;
static pthread_mutex_t rand_fd_mutex = PTHREAD_MUTEX_INITIALIZER;

void *
santa_thread(void *arg)
{
    (void) arg;

    int iterations = 0;

    printf(TAG_SANTA "is real...\n");

    while (iterations < SANTA_ITERATIONS) {
        pthread_mutex_lock(&north_pole.mutex);
        while (north_pole.helpless_elves_count < SANTA_HELPS_ELVES && north_pole.returned_reindeers != REINDEER_COUNT) {
            printf(TAG_SANTA "is sleeping\n");
            pthread_cond_wait(&north_pole.wake_santa, &north_pole.mutex);
        }
        pthread_mutex_unlock(&north_pole.mutex);

        printf(TAG_SANTA "has waken up\n");

        if (north_pole.returned_reindeers == REINDEER_COUNT) {
            pthread_mutex_lock(&north_pole.mutex);

            north_pole.returned_reindeers = 0;
            north_pole.ongoing_delivery = 1;

            pthread_cond_broadcast(&north_pole.reindeers_waiting);
            pthread_mutex_unlock(&north_pole.mutex);

            printf(TAG_SANTA "delivers presents with reindeers\n");
            sleep(2 + (rand_num() % 3));

            pthread_mutex_lock(&north_pole.mutex);
            north_pole.ongoing_delivery = 0;
            pthread_cond_broadcast(&north_pole.reindeers_delivering);
            pthread_mutex_unlock(&north_pole.mutex);

            iterations++;
        }

        if (north_pole.helpless_elves_count >= SANTA_HELPS_ELVES) {
            pthread_mutex_lock(&north_pole.mutex);
            pthread_cond_broadcast(&north_pole.santa_helping);
            pthread_mutex_unlock(&north_pole.mutex);

            printf(TAG_SANTA "helps the elves ");
            for (int i = 0; i < SANTA_HELPS_ELVES; ++i) {
                const char *fmt = i < SANTA_HELPS_ELVES - 1 ? "%zu, " : "%zu\n";
                printf(fmt, north_pole.helpless_elves[i]);
            }

            sleep(1 + (rand_num() & 1));

            pthread_mutex_lock(&north_pole.mutex);
            north_pole.helpless_elves_count = 0;
            pthread_cond_broadcast(&north_pole.elf_queue);
            pthread_mutex_unlock(&north_pole.mutex);
        }

    }

    return NULL;
}

void *
elv_thread(void *arg)
{
    if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) != 0) {
        perror("pthread_setcancelstate");
        exit(-1);
    }
    if (pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL) != 0) {
        perror("pthread_setcanceltype");
        exit(-1);
    }

    size_t elf_id = (size_t) (intptr_t) arg;

    while (1) {
//        printf(TAG_ELF "is working\n", elf_id);
        sleep(4 + (rand_num() & 1));
//        printf(TAG_ELF "has a problem\n", elf_id);

        pthread_mutex_lock(&north_pole.mutex);
        pthread_cleanup_push((void (*)(void *))pthread_mutex_unlock, &north_pole.mutex);

        while (north_pole.helpless_elves_count >= SANTA_HELPS_ELVES) {
            printf(TAG_ELF "has to wait in line for Santa\n", elf_id);
            pthread_cond_wait(&north_pole.elf_queue, &north_pole.mutex);
        }

        north_pole.helpless_elves[north_pole.helpless_elves_count] = elf_id;
        north_pole.helpless_elves_count++;

        if (north_pole.helpless_elves_count == SANTA_HELPS_ELVES) {
            printf(TAG_ELF "wakes up Santa\n", elf_id);
            pthread_cond_signal(&north_pole.wake_santa);
        } else {
            printf(TAG_ELF "is waiting for Santa (%zu is waiting)\n", elf_id, north_pole.helpless_elves_count);
        }

        assert(north_pole.helpless_elves_count <= SANTA_HELPS_ELVES);

        pthread_cond_wait(&north_pole.santa_helping, &north_pole.mutex);
        printf(TAG_ELF "is being helped by Santa\n", elf_id);

        pthread_cleanup_pop(1);
        pthread_mutex_unlock(&north_pole.mutex);

        sleep(1 + (rand_num() & 1));
    }

    return NULL;
}

_Noreturn void *
reindeer_thread(void *arg)
{
    if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) != 0) {
        perror("pthread_setcancelstate");
        exit(-1);
    }
    if (pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL) != 0) {
        perror("pthread_setcanceltype");
        exit(-1);
    }


    size_t reindeer_id = (size_t) (intptr_t) arg;

    while (1) {
//        printf(TAG_REINDEER "is on holidays\n", reindeer_id);
        sleep(5 + (rand_num() % 6));

        pthread_mutex_lock(&north_pole.mutex);
        pthread_cleanup_push((void (*)(void *))pthread_mutex_unlock, &north_pole.mutex);
        north_pole.returned_reindeers++;

        if (north_pole.returned_reindeers == REINDEER_COUNT) {
            printf(TAG_REINDEER "wakes up Santa \n", reindeer_id);
            pthread_cond_signal(&north_pole.wake_santa);
        } else {
            printf(TAG_REINDEER "waits for Santa (%zu are waiting)\n", reindeer_id, north_pole.returned_reindeers);
        }

        pthread_cond_wait(&north_pole.reindeers_waiting, &north_pole.mutex);
        pthread_cleanup_pop(1);
        pthread_mutex_unlock(&north_pole.mutex);

        printf(TAG_REINDEER "is delivering presents with Santa\n", reindeer_id);

        pthread_mutex_lock(&north_pole.mutex);
        pthread_cleanup_push((void (*)(void *))pthread_mutex_unlock, &north_pole.mutex);
        while (north_pole.ongoing_delivery) {
            pthread_cond_wait(&north_pole.reindeers_delivering, &north_pole.mutex);
        }
        pthread_cleanup_pop(1);
        pthread_mutex_unlock(&north_pole.mutex);
    }

    return NULL;
}

int
main(int argc, const char *argv[])
{
    /* Disables stdout buffering */
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
        perror("setvbuf");
        return -1;
    }

    if (pthread_create(&threads.santa, NULL, &santa_thread, NULL) != 0) {
        perror("pthread_create");
        return -1;
    }


    for (size_t i = 0; i < ELF_COUNT; i++) {
        if (pthread_create(&threads.elves[i], NULL, &elv_thread, (void *) i) != 0) {
            perror("pthread_create");
            return -1;
        }
    }

    for (size_t i = 0; i < REINDEER_COUNT; i++) {
        if (pthread_create(&threads.reindeer[i], NULL, &reindeer_thread, (void *) i) != 0) {
            perror("pthread_create");
            return -1;
        }
    }

    void *ret;
    if (pthread_join(threads.santa, &ret) != 0) {
        perror("pthread_join");
        return -1;
    }

    printf("Cancelling all threads \n");
    for (size_t i = 0; i < ELF_COUNT + REINDEER_COUNT; i++) {
        if (pthread_cancel(threads.elves_and_reindeers[i]) != 0) {
            perror("pthread_cancel");
            return -1;
        }
    }

    for (size_t i = 0; i < ELF_COUNT + REINDEER_COUNT; i++) {
        if (pthread_join(threads.elves_and_reindeers[i], &ret) != 0) {
            perror("pthread_join");
            return -1;
        }
    }

    if (rand_fd != NULL) {
        pthread_mutex_lock(&rand_fd_mutex);
        fclose(rand_fd);
        rand_fd = NULL;
        pthread_mutex_unlock(&rand_fd_mutex);
    }

    return 0;
}


static size_t
rand_num(void)
{
    /* Essentially needed for opening the file */
    pthread_mutex_lock(&rand_fd_mutex);

    if (rand_fd == NULL) {
        rand_fd = fopen("/dev/urandom", "rb");
        if (rand_fd == NULL) {
            perror("fopen");
            return rand();
        }
    }

    size_t ret;
    if (fread(&ret, sizeof(ret), 1, rand_fd) != 1) {
        perror("fread");
        return rand();
    }

    pthread_mutex_unlock(&rand_fd_mutex);

    return ret;
}