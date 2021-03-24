#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/times.h>


int
main(int argc, const char **argv)
{
    struct tms tms_before, tms_after;
    clock_t real_before, real_after;
    real_before = times(&tms_before);


    if (argc < 5) {
        static const char error[] = "usage: ./a.out <in> <out> <n1> <n2>\n";
        fwrite(error, sizeof(char), sizeof(error), stderr);
        return -1;
    }

    FILE *fin = fopen(argv[1], "r");
    FILE *fout = fopen(argv[2], "w");

    if (fin == NULL || fout == NULL) {
        static const char error[] = "error: unable to open file\n";
        fwrite(error, sizeof(char), sizeof(error), stderr);

        if (fin != NULL) {
            fclose(fin);
        }

        if (fout != NULL) {
            fclose(fout);
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
        fwrite(error, sizeof(char), sizeof(error), stderr);

        fclose(fin);
        fclose(fout);
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
        fwrite(error, sizeof(char), sizeof(error), stderr);

        fclose(fin);
        fclose(fout);
        free(table);
        return -1;
    }

    while (1) {
        ssize_t ret = fread(&buf[in_buffer], sizeof(char), buffer_size - in_buffer, fin);


        if (ret <= 0) {
            break;
        }

        in_buffer += ret;

        while (cursor < in_buffer) {
            if (pattern[status] == 0) {
                fwrite(&buf[consumed], sizeof(char), cursor - consumed - status, fout);

                consumed = cursor;

                fwrite(replace, sizeof(char), strlen(replace), fout);

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
            fwrite(&buf[consumed], sizeof(char), cursor - consumed - status, fout);
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
                fwrite(error, sizeof(char), sizeof(error), stderr);

                fclose(fin);
                fclose(fout);
                free(table);
                return -1;
            }
        }
    }

    free(buf);
    free(table);

    fclose(fin);
    fclose(fout);


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

