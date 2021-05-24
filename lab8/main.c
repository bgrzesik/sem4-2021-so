#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#define MAX_THREADS 1024

enum work_policy {
    WORK_POLICY_NUMBERS,
    WORK_POLICY_BLOCK,
};

struct image {
    size_t width;
    size_t height;
    int max_gray;
    int *data;
};

struct context {
    struct image src;
    struct image dst;
    enum work_policy policy;
    size_t threads;
    pthread_t thread_handles[MAX_THREADS];
} ctx;


void
_skip_comment(FILE *fin)
{
    char c = fgetc(fin);

    if (c != '#') {
        fseek(fin, -1, SEEK_CUR);
        return;
    }

    do {
        c = fgetc(fin);
        /* fputc(c, stderr); */
    } while (c != '\n' && c != EOF);
}


int
image_new(struct image *img, size_t width, size_t height, int max_gray)
{
    img->width = width;
    img->height = height;
    img->max_gray = max_gray;
    img->data = calloc(img->width * img->height, sizeof(int)); 

    if (img->data == NULL) {
        return -1;
    } 

    return 0;
}


int
image_read(struct image *img, FILE *fin)
{
    char magic[3];
    fscanf(fin, "%3s\n", magic);

    if (strcmp(magic, "P2") != 0) {
        fprintf(stderr, "error: unsupported file format");
        return -1;
    }

    _skip_comment(fin);
    if (fscanf(fin, "%zu %zu ", &img->width, &img->height) < 0) {
        perror("fscanf");
        return -1;
    }

    _skip_comment(fin);
    if (fscanf(fin, "%d ", &img->max_gray) < 0) {
        perror("fscanf");
        return -1;
    }

    img->data = calloc(img->width * img->height, sizeof(int)); 
    if (img->data == NULL) {
        return -1;
    }

    for (int row = 0; row < img->height; row++) {
        for (int col = 0; col < img->width; col++) {
            if (fscanf(fin, "%d", &img->data[row * img->width + col]) < -1) {
                perror("fscanf");
                return -1;
            }
        }
    }

    return 0;
}

int
image_write(struct image *img, FILE *fout)
{
    if (fprintf(fout, "P2\n%zu %zu\n%d\n", 
                img->width, img->height, img->max_gray) < 0) {
        return -1;
    }

    for (int row = 0; row < img->height; row++) {
        for (int col = 0; col < img->width; col++) {
            size_t i = row * img->width + col;

            int gray = img->data[i];
            if (fprintf(fout, "%d ", gray) < 0) {
                return -1;
            }

            if (i % 16 == 15 && fprintf(fout, "\n") < 0) {
                return -1;
            }
        }
    }

    return 0;
}

void
image_free(struct image *img)
{
    if (img->data != NULL) {
        free(img->data);
        img->data = NULL;
    }
}


void *
worker_thread(void *arg)
{
    size_t thread_id = (size_t) (intptr_t) arg;

    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        return NULL;
    }

    if (ctx.policy == WORK_POLICY_NUMBERS) {
        int gray_range = ctx.src.max_gray / ctx.threads;
        int gray_min = thread_id * gray_range;
        int gray_max = (thread_id + 1) * gray_range;

        if (thread_id == 0) {
            gray_min = 0;
        } else if (thread_id == ctx.threads - 1) {
            gray_max = ctx.src.max_gray + 1;
        }


        for (int row = 0; row < ctx.src.height; row++) {
            for (int col = 0; col < ctx.src.width; col++) {
                int gray = ctx.src.data[row * ctx.src.width + col];

                if (gray_min <= gray && gray < gray_max) {
                    gray = ctx.src.max_gray - gray;
                    ctx.dst.data[row * ctx.src.width + col] = gray;
                }
            }
        }


    } else if (ctx.policy == WORK_POLICY_BLOCK) {
        int range = ctx.src.width * ctx.src.height;
        int start = thread_id * range / ctx.threads;
        int end = (thread_id + 1) * range / ctx.threads;

        if (thread_id == 0) {
            start = 0;
        } else if (thread_id == ctx.threads - 1) {
            end = range;
        }


        for (int i = start; i < end; i++) {
            int gray = ctx.src.data[i];
            gray = ctx.src.max_gray - gray;
            ctx.dst.data[i] = gray;
        }
    }

    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        return NULL;
    }

    return (void *) (intptr_t) (end.tv_nsec - start.tv_nsec);
}


int
main(int argc, const char *argv[])
{
    if (argc != 5) {
        fprintf(stderr, "usage: ./a.out <threads> <policy> <in> <out>\n");
        return -1;
    }

    if ((ctx.threads = atoi(argv[1])) <= 0 || ctx.threads > MAX_THREADS) {
        fprintf(stderr, "error: invalid threads argument\n");
        return -1;
    }

    if (strcmp(argv[2], "numbers") == 0) {
        ctx.policy = WORK_POLICY_NUMBERS;
    } else if (strcmp(argv[2], "block") == 0) {
        ctx.policy = WORK_POLICY_BLOCK;
    } else {
        fprintf(stderr, "error: invalid policy argument\n");
        return -1;
    }

    FILE *fin = fopen(argv[3], "r");
    if (fin == NULL) {
        perror("fopen");
        fprintf(stderr, "error: unable to open '%s' for reading\n", argv[3]);
        return -1;
    }

    if (image_read(&ctx.src, fin) != 0) {
        fclose(fin);
        return -1;
    }

    if (image_new(&ctx.dst, ctx.src.width, ctx.src.height, ctx.src.max_gray) != 0) {
        return -1;
    }

    fclose(fin);

    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        return -1;
    }

    for (size_t i = 0; i < ctx.threads; i++) {
        int ret = pthread_create(&ctx.thread_handles[i], NULL, 
                                 &worker_thread, (void *) (intptr_t) i);

        if (ret != 0) {
            perror("pthread_create");
            return -1;
        }
    }
    

    long total_time = 0;
    for (size_t i = 0; i < ctx.threads; i++) {
        void *result;
        if (pthread_join(ctx.thread_handles[i], &result) != 0) {
            perror("pthread_join");
            return -1;
        }

        long time = (long) (intptr_t) result;
        total_time += time;

        printf("Thread %3zu took %8ld ns\n", i, time);
    }

    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        return -1;
    }

    printf("Total thread time: %10ld ns\n", total_time);
    printf("Total time: %10ld ns\n", end.tv_nsec - start.tv_nsec);

    FILE *fout = fopen(argv[4], "w");
    if (fin == NULL) {
        perror("fopen");
        fprintf(stderr, "error: unable to open '%s' for writing\n", argv[4]);
        return -1;
    }

    if (image_write(&ctx.dst, fout) != 0) {
        return -1;
    }

    fclose(fout);

    image_free(&ctx.src);
    image_free(&ctx.dst);

    return 0;
}
