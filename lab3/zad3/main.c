#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static inline int
append(char *begin, char **end, const char *suffix, size_t max)
{
    size_t len = strlen(suffix);

    if (max - (*end - begin + 1) < len) {
        fprintf(stderr, "error: buffer overflow\n");
        return -1;
    }

    memcpy(*end, suffix, len);
    *end += len;
    (*end)[0] = 0;

    return 0;
}

static size_t *
kmp_table(const char *pattern, size_t len)
{
    size_t *table = calloc(len, sizeof(size_t));
    if (table == NULL) {
        fprintf(stderr, "error: calloc failed\n");
        return NULL;
    }

    int j = 0;
    for (int i = 1; i < len; i++) {
        if (pattern[i] == pattern[j]) {
            j++;
            table[i] = j;
        } else {
            j = table[j];
        }
    }

    return table;
}

static int
kmp_find(const char *path, const char *pattern, size_t *table, size_t len)
{
    size_t cursor = 0;
    size_t buffer_size = 256;
    size_t in_buffer = 0;
    size_t consumed = 0;
    size_t status = 0;

    char *buf = calloc(buffer_size, sizeof(char));
    if (buf == NULL) {
        fprintf(stderr, "error: calloc failed\n");
        return -1;
    }

    FILE *fin = fopen(path, "r");
    if (fin == NULL) {
        fprintf(stderr, "error: unable to open %s\n", path);
        free(buf);
        return -1;
    }


    while (1) {
        ssize_t ret = fread(&buf[in_buffer], sizeof(char), buffer_size - in_buffer, fin);

        if (ret <= 0) {
            break;
        }

        in_buffer += ret;

        while (cursor < in_buffer) {
            if (pattern[status] == 0) {
                fclose(fin);
                free(buf);
                return 1;
            }

            if (buf[cursor] == pattern[status]) {
                status++;
            } else {
                while (status > 0 && pattern[status] != buf[cursor]) {
                    status = table[status - 1];
                }
            }

            cursor++;
        }

        if (cursor - consumed - status > 32) {
            consumed = cursor - status;
        }

        if (consumed > 32) {
            memmove(&buf[0], &buf[cursor], in_buffer - consumed);
            cursor -= consumed;
            in_buffer -= consumed;
            consumed = 0;
        }
        
        if (buffer_size - in_buffer < 32) {
            buffer_size <<= 1;
            buf = realloc(buf, buffer_size);

            if (buf == NULL) {
                fprintf(stderr, "error: realloc failed\n");
                fclose(fin);
                free(buf);
                return -1;
            }
        }
    }

    fclose(fin);
    free(buf);

    return 0;
}

static int
is_txt_file(const char *file) 
{
    char *ext = strrchr(file, '.');

    static const char *exts[] = {
        ".txt", ".h", ".c", ".cpp", ".hpp", ".java", ".py", ".rs",
        ".json", ".xml", ".html",
    };

    for (size_t i = 0; i < sizeof(exts) / sizeof(exts[0]); i++) {
        if (strcmp(ext, exts[i]) == 0) {
            //printf("file %s\n", file);
            return 1;
        }
    }

    return 0;
}

int
main(int argc, const char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: ./a.out <dir> <pattern>\n");
        return -1;
    }

    char pathbuf[256];
    char *prefix = pathbuf;

    if (append(pathbuf, &prefix, argv[1], sizeof(pathbuf)) != 0) {
        return -1;
    }

    if (append(pathbuf, &prefix, "/", sizeof(pathbuf)) != 0) {
        return -1;
    }

    size_t pattern_len = strlen(argv[2]);
    size_t *pattern = NULL;

    DIR *dir = opendir(argv[1]);
    if (dir == NULL) {
        return -1;
    }

    struct dirent *ent = NULL;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, "..") == 0 || strcmp(ent->d_name, ".") == 0) {
            continue;
        } 

        char *cur = prefix;
        if (append(pathbuf, &cur, ent->d_name, sizeof(pathbuf)) != 0) {
            if (pattern != NULL) {
                free(pattern);
            }
            closedir(dir);

            return -1;
        }

        if (ent->d_type == DT_DIR && fork() == 0) {
            if (pattern != NULL) {
                free(pattern);
            }
            closedir(dir);

            execl(argv[0], argv[0], pathbuf, argv[2], NULL);

            return -1;
        } else if (ent->d_type == DT_REG && is_txt_file(pathbuf) != 0) {
            if (pattern == NULL) {
                pattern = kmp_table(argv[2], pattern_len);

                if (pattern == NULL) {
                    closedir(dir);
                    return -1;
                }
            }

            int ret = kmp_find(pathbuf, argv[2], pattern, pattern_len);

            if (ret == -1) {
                free(pattern);
                closedir(dir);
                return -1;
            }

            if (ret != 0) {
                printf("%d: entry %s\n", getpid(), pathbuf);
            }
        }

        //printf("// %d: entry %s\n", getpid(), pathbuf);
    }

    int ret;
    while(wait(&ret) > 0) {
        if (WEXITSTATUS(ret) != 0) {
            if (pattern != NULL) {
                free(pattern);
            }
            closedir(dir);

            return WEXITSTATUS(ret);
        }
    }

    if (pattern != NULL) {
        free(pattern);
    }
    closedir(dir);

    return 0;
}
