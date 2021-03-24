#include <unistd.h>
#include <fcntl.h>
#include <string.h>
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
_print_num(int fd, int num)
{
    int dec = (int) pow(10, (int) log10(num));
    do {
        int a = 0;

        if (dec != 0) {
            a = (num / dec) % 10;
        }

        char digit = '0' + a;

        write(fd, &digit, sizeof(digit));

        dec /= 10;
    } while (dec != 0);
}


int
main(int argc, const char **argv)
{
    int file = open("data.txt", O_RDONLY);
    if (file == -1) {
        static const char no_file[] = "error: unable to open file data.txt\n";
        write(STDERR_FILENO, no_file, sizeof(no_file));
        return -1;
    }

    const static mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

    int file_a = open("a.txt", O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (file_a == -1) {
        static const char no_file[] = "error: unable to open file a.txt\n";
        write(STDERR_FILENO, no_file, sizeof(no_file));
        return -1;
    }

    int file_b = open("b.txt", O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (file_b == -1) {
        static const char no_file[] = "error: unable to open file b.txt\n";
        write(STDERR_FILENO, no_file, sizeof(no_file));
        return -1;
    }

    int file_c = open("c.txt", O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (file_c == -1) {
        static const char no_file[] = "error: unable to open file c.txt\n";
        write(STDERR_FILENO, no_file, sizeof(no_file));
        return -1;
    }

    struct cursor cur = { sizeof(cur.buf), sizeof(cur.buf), 0 };
    char buf[256];

    int even_count = 0;

    while (_read_line(file, &cur, buf, sizeof(buf)) == 0) {
        long long num;
        
        if (_parse_num(buf, sizeof(buf), &num) != 0) {
            static const char data_err[] = "error: data error\n";
            write(STDERR_FILENO, data_err, sizeof(data_err));
            return -1;
        }

        if (num % 2 == 0) {
            even_count++;
        }

        int dec = (num / 10) % 10;

        if (dec == 0 || dec == 7) {
            write(file_b, buf, strlen(buf));
        }

        long long num_sqrt = sqrt(num);

        if (num_sqrt * num_sqrt == num) {
            write(file_c, buf, strlen(buf));
        }

    }

    static const char even[] = "Liczb parzystych jest ";
    write(file_a, even, sizeof(even));

    _print_num(file_a, even_count);

    static const char nl[] = "\n";
    write(file_a, nl, sizeof(nl));
    
    
    close(file);
    close(file_a);
    close(file_b);
    close(file_c);


    return 0;
}
