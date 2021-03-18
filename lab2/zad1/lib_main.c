
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 10

struct cursor {
    char buf[BUF_SIZE];
    size_t read;
    size_t cursor;
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


int
main(int argc, const char **argv)
{
    FILE *lfile = fopen("a.txt", "r");
    FILE *rfile = fopen("b.txt", "r");

    if (lfile == NULL || rfile == NULL) {
        fprintf(stderr, "error: unable to open file\n");
        return -1;
    }

    struct cursor lcur;
    lcur.cursor = lcur.read = sizeof(lcur.buf);

    struct cursor rcur;
    lcur.cursor = rcur.read = sizeof(rcur.buf);
    
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

