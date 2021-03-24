#include <unistd.h>
#include <sys/times.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#ifndef BUF_SIZE
#define BUF_SIZE 512
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif


struct cursor {
    size_t read;
    size_t cursor;
    int eof;
    char buf[BUF_SIZE];
};

static int
_read_line(int fd, struct cursor *cur, char *out, size_t out_len)
{
    while (!cur->eof || cur->cursor < cur->read) {
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
                *out = 0;

                return 0;
            }
        }

        cur->read = read(fd, cur->buf, sizeof(cur->buf));
        cur->cursor = 0;
        
        if (cur->read <= 0) {
            if (cur->read == 0) {
                cur->eof = 1;
            }

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
        write(STDERR_FILENO, no_args, sizeof(no_args));
        return -1;
    }

    int file = open(argv[2], O_RDONLY);
    if (file == -1) {
        static const char no_file[] = "error: unable to open file\n";
        write(STDERR_FILENO, no_file, sizeof(no_file));
        return -1;
    }

    struct cursor cur = { sizeof(cur.buf), sizeof(cur.buf), 0 };
    char buf[256];
    

    while (_read_line(file, &cur, buf, sizeof(buf)) == 0) {
        size_t len = strlen(buf);

        if (memchr(buf, argv[1][0], len) != NULL) {
            write(STDOUT_FILENO, buf, len);
        }
    }
    
    close(file);


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
