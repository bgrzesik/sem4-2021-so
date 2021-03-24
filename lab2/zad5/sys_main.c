#include <unistd.h>
#include <sys/times.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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
        write(STDERR_FILENO, error, sizeof(error));
        return -1;
    }

    int fin = open(argv[1], O_RDONLY);

    static const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
    int fout = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, mode);

    if (fin == -1 || fout == -1) {
        static const char error[] = "usage: ./a.out <in> <out>\n";
        write(STDERR_FILENO, error, sizeof(error));
        
        if (fin == -1) {
            close(fin);
        }

        if (fout == -1) {
            close(fout);
        }


        return -1;
    }

    ssize_t bytes;

    char buf[BUF_SIZE];
    char *end;

    size_t remainder = 0;

    while ((bytes = read(fin, buf, sizeof(buf))) > 0) {
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

            write(fout, cursor, next - cursor);

            if (put_nl) {
                static const char newline[] = "\n";
                write(fout, newline, sizeof(newline));
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

