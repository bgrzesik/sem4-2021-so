
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#define BUF_SIZE 10

struct cursor {
    char buf[BUF_SIZE];
    size_t read;
    size_t cursor;
    int eof;
};

static int
_read_line(int fd, struct cursor *cur)
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

            write(STDOUT_FILENO, buf, len);
            cur->cursor += len;

            if (nl != NULL) {
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
    int lfile = open("a.txt", O_RDONLY);
    int rfile = open("b.txt", O_RDONLY);

    struct cursor lcur;
    lcur.cursor = lcur.read = sizeof(lcur.buf);
    lcur.eof = 0;

    struct cursor rcur;
    rcur.cursor = rcur.read = sizeof(rcur.buf);
    rcur.eof = 0;
    
    int lret = 0;
    int rret = 0;

    while (lret == 0 || rret == 0) {
        if (lret == 0) {
            lret = _read_line(lfile, &lcur);
        }
        if (rret == 0) {
            rret = _read_line(rfile, &rcur);
        }
    }

    close(lfile);
    close(rfile);

    return 0;
}

