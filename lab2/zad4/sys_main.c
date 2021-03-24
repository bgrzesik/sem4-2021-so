#include <unistd.h>
#include <sys/times.h>

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


int
main(int argc, const char **argv)
{
    struct tms tms_before, tms_after;
    clock_t real_before, real_after;
    real_before = times(&tms_before);

    if (argc < 5) {
        static const char error[] = "usage: ./a.out <in> <out> <n1> <n2>\n";
        write(STDERR_FILENO, error, sizeof(error));
        return -1;
    }

    int fin = open(argv[1], O_RDONLY);

    static const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
    int fout = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, mode);

    if (fin == -1 || fout == -1) {
        static const char error[] = "error: unable to open file\n";
        write(STDERR_FILENO, error, sizeof(error));

        if (fin != -1) {
            close(fin);
        }

        if (fout != -1) {
            close(fout);
        }

        return -1;
    }
    
    size_t cursor = 0;
    size_t buffer_size = 256;
    size_t in_buffer = 0;
    size_t consumed = 0;

    const char *pattern = argv[3];
    const char *replace = argv[4];

    size_t len = strlen(pattern);

    size_t *table = calloc(len, sizeof(size_t));
    if (table == NULL) {
        static const char error[] = "error: calloc failed\n";
        write(STDERR_FILENO, error, sizeof(error));

        close(fin);
        close(fout);
        return -1;
    }

    int j = 0;
    for (int i = 1; i < len; i++) {
        if (pattern[i] == pattern[j]) {
            j++;
            table[i] = j;
        } else {
            j = table[j];
        }
    }

    size_t status = 0;
    
    char *buf = calloc(buffer_size, sizeof(char));
    if (buf == NULL) {
        static const char error[] = "error: calloc failed\n";
        write(STDERR_FILENO, error, sizeof(error));

        free(table);
        close(fin);
        close(fout);
        return -1;
    }

    while (1) {
        ssize_t ret = read(fin, &buf[in_buffer], buffer_size - in_buffer);

        if (ret <= 0) {
            break;
        }

        in_buffer += ret;

        while (cursor < in_buffer) {
            if (pattern[status] == 0) {
                write(fout, &buf[consumed], cursor - consumed - status);

                consumed = cursor;

                write(fout, replace, strlen(replace));

                status = 0; /* TODO rethink */

                continue;
            }

            if (buf[cursor] == pattern[status]) {
                status++;
            } else {
                while (status > 0 && pattern[status] != buf[cursor]) {
                    status = table[status - 1];
                }
            }
            cursor++;
        }

        if (cursor - consumed - status > 32) {
            write(fout, &buf[consumed], cursor - consumed - status);
            consumed = cursor - status;
        }

        if (consumed > 32) {
            memmove(&buf[0], &buf[cursor], in_buffer - consumed);
            cursor -= consumed;
            in_buffer -= consumed;
            consumed = 0;
        }
        
        if (buffer_size - in_buffer < 32) {
            buffer_size <<= 1;
            buf = realloc(buf, buffer_size);

            if (buf == NULL) {
                static const char error[] = "error: realloc failed\n";
                write(STDERR_FILENO, error, sizeof(error));

                free(table);
                close(fin);
                close(fout);
                return -1;
            }
        }
    }


    free(buf);
    free(table);

    close(fin);
    close(fout);


    real_after = times(&tms_after);

    clock_t rtime = real_after - real_before;
    clock_t utime = tms_after.tms_utime - tms_before.tms_utime;
    clock_t stime = tms_after.tms_stime - tms_before.tms_stime;

    float clk_tck = (float) sysconf(_SC_CLK_TCK);

    fprintf(stderr, "real time: %4zu %7.3fs \n", rtime, rtime / clk_tck);
    fprintf(stderr, "user time: %4zu %7.3fs \n", utime, utime / clk_tck);
    fprintf(stderr, "sys  time: %4zu %7.3fs \n\n", stime, stime / clk_tck);

    return 0;
}

