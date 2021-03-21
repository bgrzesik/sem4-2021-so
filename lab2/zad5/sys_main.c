#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#ifndef BUF_SIZE
#define BUF_SIZE 256
#endif

#ifndef LINE_WRAP
#define LINE_WRAP 50
#endif


int
main(int argc, const char **argv)
{
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

    return 0;
}

