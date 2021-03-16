
#include "merge.h"

#include <stdio.h>
#include <stdlib.h>


FILE *merge_files(FILE *left, FILE *right)
{
    FILE *tmp = tmpfile();
    if (tmp == NULL) {
        return NULL;
    }

    char *line_left = NULL, *line_right = NULL;
    size_t len_left = 0, len_right = 0;
    
    while (1) {
        int read_left = getline(&line_left, &len_left, left);
        int read_right = getline(&line_right, &len_right, right);

        if (read_left == -1 || read_right == -1) {
            break;
        }

        fwrite(line_left,  read_left, sizeof(char), tmp);
        fwrite(line_right, read_right, sizeof(char), tmp);
    }

    if (line_left != NULL) {
        free(line_left);
    }

    if (line_right != NULL) {
        free(line_right);
    }

    fseek(tmp, 0, SEEK_SET);

    return tmp;
}


