#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

static int
_parse_num(char *buf, size_t size, long long *out)
{
    *out = 0;

    while (size > 0) {
        if (*buf == 0 || *buf == '\n') {
            return 0;
        }

        if (*buf < '0' || *buf > '9') {
            break;
        }

        *out *= 10;
        *out += *buf - '0';

        buf++;
        size--;
    }

    return -1;
}


static void
_print_num(FILE *file, int num)
{
    int dec = (int) pow(10, (int) log10(num));
    do {
        int a = 0;

        if (dec != 0) {
            a = (num / dec) % 10;
        }

        char digit = '0' + a;

        fwrite(&digit, sizeof(digit), 1, file);

        dec /= 10;
    } while (dec != 0);
}

int
main(int argc, const char **argv)
{
    FILE *file = fopen("data.txt", "r");
    if (file == NULL) {
        static const char no_file[] = "error: unable to open file data.txt\n";
        fwrite(no_file, sizeof(char), sizeof(no_file), stderr);
        return -1;
    }

    FILE *file_a = fopen("a.txt", "w");
    if (file_a == NULL) {
        static const char no_file[] = "error: unable to open file a.txt\n";
        fwrite(no_file, sizeof(char), sizeof(no_file), stderr);
        return -1;
    }

    FILE *file_b = fopen("b.txt", "w");
    if (file_b == NULL) {
        static const char no_file[] = "error: unable to open file b.txt\n";
        fwrite(no_file, sizeof(char), sizeof(no_file), stderr);
        return -1;
    }

    FILE *file_c = fopen("c.txt", "w");
    if (file_c == NULL) {
        static const char no_file[] = "error: unable to open file c.txt\n";
        fwrite(no_file, sizeof(char), sizeof(no_file), stderr);
        return -1;
    }


    struct cursor cur = { sizeof(cur.buf), sizeof(cur.buf) };
    char buf[256];
    
    size_t even_count = 0;

    while (_read_line(file, &cur, buf, sizeof(buf)) == 0) {
        long long num;
        
        if (_parse_num(buf, sizeof(buf), &num) != 0) {
            static const char data_err[] = "error: data error\n";
            fwrite(data_err, sizeof(char), sizeof(data_err), stderr);
            return -1;
        }

        if (num % 2 == 0) {
            even_count++;
        }

        int dec = (num / 10) % 10;

        if (dec == 0 || dec == 7) {
            fwrite(buf, 1, strlen(buf), file_b);
        }

        long long num_sqrt = sqrt(num);

        if (num_sqrt * num_sqrt == num) {
            fwrite(buf, 1, strlen(buf), file_c);
        }

    }


    static const char even[] = "Liczb parzystych jest ";
    fwrite(even, sizeof(char), sizeof(even), file_a);

    _print_num(file_a, even_count);

    static const char nl[] = "\n";
    fwrite(nl, sizeof(char), sizeof(nl), file_a);
    
    
    fclose(file);
    fclose(file_a);
    fclose(file_b);
    fclose(file_c);

    return 0;
}
