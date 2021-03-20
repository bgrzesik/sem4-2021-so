#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
    if (argc < 3) {
        fprintf(stderr, "usage: %s <file> <char>\n", argv[0]);
        return -1;
    }

    FILE *file = fopen(argv[2], "r");
    if (file == NULL) {
        fprintf(stderr, "error: unable to open %s\n", argv[2]);
        return -1;
    }

    struct cursor cur = { sizeof(cur.buf), sizeof(cur.buf) };
    char buf[256];
    

    while (_read_line(file, &cur, buf, sizeof(buf)) == 0) {
        if (strchr(buf, argv[1][0]) != NULL) {
            fputs(buf, stdout);
        }
    }
    


    return 0;
}
