#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/times.h>

#ifndef BUF_SIZE
#define BUF_SIZE 256
#endif

#ifndef LINE_WRAP
#define LINE_WRAP 50
#endif


int
main(int argc, const char **argv)
{
    struct tms tms_before, tms_after;
    clock_t real_before, real_after;
    real_before = times(&tms_before);

    if (argc != 3) {
        static const char error[] = "usage: ./a.out <in> <out>\n";
        fwrite(error, sizeof(char), sizeof(error), stderr);
        return -1;
    }

    FILE *fin = fopen(argv[1], "r");
    FILE *fout = fopen(argv[2], "w");

    if (fin == NULL || fout == NULL) {
        static const char error[] = "usage: ./a.out <in> <out>\n";
        fwrite(error, sizeof(char), sizeof(error), stderr);
        
        if (fin == NULL) {
            fclose(fin);
        }

        if (fout == NULL) {
            fclose(fout);
        }

        return -1;
    }

    ssize_t bytes;

    char buf[BUF_SIZE];
    char *end;

    size_t remainder = 0;

    while ((bytes = fread(buf, sizeof(char), sizeof(buf), fin)) > 0) {
        char *cursor = buf;
        end = buf + bytes;

        while (cursor < end) {
            char *next = cursor + LINE_WRAP - remainder;
            int put_nl = 1;

            if (next > end) {
                next = end;
                put_nl = 0;
            }

            char *nl = memchr(cursor, '\n', next - cursor);
            if (nl != NULL) {
                next = nl + 1;
                put_nl = 0;
            }

            fwrite(cursor, sizeof(char), next - cursor, fout);

            if (put_nl) {
                static const char newline[] = "\n";
                fwrite(newline, sizeof(char), sizeof(newline), fout);
            }

            if (next == end) {
                remainder = end - cursor;
                break;
            } else {
                remainder = 0;
            }

            cursor = next;
        }

    }

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

