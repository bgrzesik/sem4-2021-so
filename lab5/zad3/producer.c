

#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>


#include "common.h"

size_t
read_no_newline(char *to, size_t n, FILE *fin)
{
    int nread = 0;
    
    while (n > 0) {
        int bytes = fread(to, 1, 1, fin);

        if (bytes <= 0) {
            break;
        }
        
        if (*to != '\n') {
            n -= 1;
            to++;
            nread += 1;
        }
    }

    *to = 0;

    return nread;
}

int
main(int argc, const char *argv[])
{
    if (argc != 5) {
        fprintf(stderr, "usage: ./a.out <fifo> <line_no> <in> <N>\n");
        return -1;
    }

    struct fifo fifo;
    if (open_fifo(&fifo, argv[1], O_WRONLY) == -1) {
        return -1;
    }

    int line_no = atoi(argv[2]);
    if (line_no == 0 && errno != 0) {
        perror("atoi");
        return -1;
    }

    FILE *fin = fopen(argv[3], "r");
    if (fin == NULL) {
        perror("fopen");
        return -1;
    }

    int n = atoi(argv[4]);
    if (n == 0) {
        perror("atoi");
        return -1;
    }

    struct packet *packet;
    packet = (struct packet *) malloc(sizeof(struct packet) + n + 1);
    packet->line_no = line_no;

    if (packet == NULL) {
        perror("malloc");
        return -1;
    }

    int bytes;
    while ((bytes = read_no_newline(packet->data, n, fin)) > 0) {
        packet->data[bytes] = 0;
        /* printf("bytes = %d, %s\n", bytes, packet->data); */

        bytes = write_fifo(&fifo, packet, sizeof(struct packet) + n);
        /* printf("%zu %s\n", packet->line_no, packet->data); */

        if (bytes <= 0) {
            break;
        }

        sleep(2);
    }

    close_fifo(&fifo);
    fclose(fin);
    free(packet);

    return 0;
}
