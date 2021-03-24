
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef BUF_SIZE
#define BUF_SIZE 512
#endif

struct cursor {
    size_t read;
    size_t cursor;
    char buf[BUF_SIZE];
};

static int
_read_line(FILE *file, struct cursor *cur)
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

            fwrite(buf, sizeof(char), len, stdout);
            cur->cursor += len;

            if (nl != NULL) {
                if (feof(file) && nl == NULL) {
                    fwrite("\n", sizeof(char), 1, stdout);
                }

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

static int
_read_stdin(char *buf, size_t size)
{
    size_t cur = 0;

    while (cur < size) {
        int ret = fread(buf, sizeof(char), 1, stdin);

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

    struct cursor stdin_cur = { sizeof(stdin_cur.buf), sizeof(stdin_cur.buf) };

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

    FILE *lfile = fopen(lname, "r");
    FILE *rfile = fopen(rname, "r");

    if (lfile == NULL || rfile == NULL) {
        static const char no_file[] = "error: unable to open file\n";
        fwrite(no_file, sizeof(char), sizeof(no_file), stderr);
        return -1;
    }

    struct cursor lcur = { sizeof(lcur.buf), sizeof(lcur.buf) };
    struct cursor rcur = { sizeof(rcur.buf),  sizeof(rcur.buf) };
    
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

    fclose(lfile);
    fclose(rfile);

    return 0;
}

