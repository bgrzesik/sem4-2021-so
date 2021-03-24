#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/times.h>

#ifndef BUF_SIZE
#define BUF_SIZE 512
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif


struct cursor {
    size_t read;
    size_t cursor;
    char buf[BUF_SIZE];
};

static int
_read_line(FILE *file, struct cursor *cur, char *out, size_t out_len)
{
    while (!feof(file) || cur->cursor < cur->read) {
        if (cur->cursor < cur->read) {
            char *buf = cur->buf + cur->cursor;
            char *nl = memchr(buf, '\n', cur->read - cur->cursor);
            
            size_t len;
            if (nl != NULL) {
                len = nl - buf + 1;
            } else {
                len = cur->read - cur->cursor;
            }

            memcpy(out, buf, min(len, out_len));
            out = &out[len];
            out_len -= len;

            cur->cursor += len;

            if (nl != NULL) {
                if (feof(file) && nl == NULL) {
                    *out = '\n';
                    out++;
                }

                *out = 0;

                return 0;
            }
        }

        cur->read = fread(cur->buf, sizeof(char), sizeof(cur->buf), file);
        cur->cursor = 0;
        
        if (cur->read <= 0) {
            break;
        }
    }

    return -1;
}

int
main(int argc, const char **argv)
{
    struct tms tms_before, tms_after;
    clock_t real_before, real_after;
    real_before = times(&tms_before);
    

    if (argc < 3) {
        static const char no_args[] = "usage: ./a.out <file> <char>\n";
        fwrite(no_args, sizeof(char), sizeof(no_args), stderr);
        return -1;
    }

    FILE *file = fopen(argv[2], "r");
    if (file == NULL) {
        static const char no_file[] = "error: unable to open file\n";
        fwrite(no_file, sizeof(char), sizeof(no_file), stderr);
        return -1;
    }

    struct cursor cur = { sizeof(cur.buf), sizeof(cur.buf) };
    char buf[256];
    

    while (_read_line(file, &cur, buf, sizeof(buf)) == 0) {
        size_t len = strlen(buf);

        if (memchr(buf, argv[1][0], len) != NULL) {
            fwrite(buf, 1, len, stdout);
        }
    }
    
    
    fclose(file);


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
