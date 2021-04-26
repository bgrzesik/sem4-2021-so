
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "list.h"


int
append_line(const char *file, int line_no, char *appendix)
{
    /* if failed will catch later */
    if (access(file, R_OK | W_OK) != 0) {
        creat(file, 0666);
    }

    int lock = open(file, O_RDONLY);
    if (flock(lock, LOCK_EX) != 0) {
        perror("fflock");
        return -1;
    }

    FILE *fout = fopen(file, "r");
    if (fout == NULL) {
        perror("fopen");
        return -1;
    }


    struct list lines;
    list_init(&lines, sizeof(struct list));

    char *line = NULL;
    size_t line_size = 0;

    while (getline(&line, &line_size, fout) > 0) {
        struct list *entry = (struct list *) list_append(&lines);
        list_init(entry, sizeof(char));

        char *c = line;
        while (*c != 0) {
            if (*c == '\n') {
                c++;
                continue;
            }

            LIST_EMBRACE(entry, char, *c);

            c++;
        }

        LIST_EMBRACE(entry, char, 0);
    }

    if (line != NULL) {
        free(line);
    }

    for (size_t i = list_size(&lines); i < line_no + 1; i++) {
        struct list *entry = (struct list *) list_append(&lines);
        list_init(entry, sizeof(char));
        LIST_EMBRACE(entry, char, 0);
    }

    fclose(fout);

    struct list *list = (struct list *) list_get(&lines, line_no);

    while (*appendix != 0) {
        char *c = list_insert(list, list_size(list) - 1);
        *c = *appendix;
        appendix++;
    }

    fout = fopen(file, "w");
    if (fout == NULL) {
        perror("fopen");
        return -1;
    }

    LIST_FOREACH(&lines, struct list, elem) {
        fwrite(elem->data, sizeof(char), list_size(elem) - 1, fout);
        fwrite("\n", sizeof(char), 1, fout);
        list_free(elem);
    }

    list_free(&lines);
    fclose(fout);

    if (flock(lock, LOCK_UN) != 0) {
        perror("fflock");
        return -1;
    }

    return 0;
}


int
main(int argc, const char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "usage: ./a.out <fifo> <out> <N>\n");
        return -1;
    }

    struct fifo fifo;
    if (open_fifo(&fifo, argv[1], O_RDONLY) == -1) {
        return -1;
    }
    
    const char *file = argv[2];

    int n = atoi(argv[3]);
    if (n == 0) {
        perror("atoi");
        return -1;
    }

    struct packet *packet;
    packet = (struct packet *) malloc(sizeof(struct packet) + n + 1);

    if (packet == NULL) {
        perror("malloc");
        return -1;
    }

    int total = 0;
    int bytes;

    do {
        bytes = read_fifo(&fifo, packet, sizeof(struct packet) + n);
        if (bytes <= 0) {
            break;
        }

        /* printf("%zu %s\n", packet->line_no, packet->data); */
        if (append_line(file, packet->line_no, packet->data) != 0) {
            return -1;
        }

        total += bytes;
        sleep(2);
    } while (bytes > 0);

    /* printf("total = %d\n", total); */

    close_fifo(&fifo);
    free(packet);

    return 0;
}
