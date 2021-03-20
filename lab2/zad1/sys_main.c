
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#ifndef BUF_SIZE
#define BUF_SIZE 512
#endif

struct cursor {
    size_t read;
    size_t cursor;
    int eof;
    char buf[BUF_SIZE];
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


static int
_read_stdin(char *buf, size_t size)
{
    size_t cur = 0;

    while (cur < size) {
        int ret = read(STDIN_FILENO, buf, sizeof(char));

        if (ret != 1) {
            return -1;
        }

        if (*buf == 0 || *buf == '\n') {
            *buf = 0;
            return 0;
        }

        if (*buf != ' ' && *buf != '\n' && *buf != '\r') {
            buf++;
        }
    }

    return -1;
}


int
main(int argc, const char **argv)
{
    char lname_buf[256], rname_buf[256];
    const char *lname, *rname;

    struct cursor stdin_cur = { sizeof(stdin_cur.buf), sizeof(stdin_cur.buf), 0 };

    if (argc >= 2) {
        lname = argv[1];
    } else {
        int ret = _read_stdin(lname_buf, sizeof(rname_buf));
        if (ret != 0) {
            return ret;
        }
        lname = &lname_buf[0];
    }

    if (argc >= 3) {
        rname = argv[2];
    } else {
        int ret = _read_stdin(rname_buf, sizeof(rname_buf));
        if (ret != 0) {
            return ret;
        }
        rname = &rname_buf[0];
    }

    int lfile = open(lname, O_RDONLY);
    int rfile = open(rname, O_RDONLY);

    if (lfile == -1 || rfile == -1) {
        static const char no_file[] = "error: unable to open file\n";
        write(STDERR_FILENO, no_file, sizeof(no_file));
        return -1;
    }

    struct cursor lcur = { sizeof(lcur.buf), sizeof(lcur.buf), 0 };
    struct cursor rcur = { sizeof(rcur.buf), sizeof(rcur.buf), 0 };
    
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

