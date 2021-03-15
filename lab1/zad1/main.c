
#include <time.h>
#include <sys/times.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>

#include "vp_list.h"



void
vp_list_debug_print(struct vp_list *arr)
{
    printf("arr.size = %ld\n", arr->size); 
    printf("arr.capacity = %ld\n", arr->capacity); 
    for (size_t i = 0; i < arr->size; i++) {
        printf("arr->array[%ld] = %s\n", i, (char *) vp_list_get(arr, i));
    }
}


int 
main(int argc, const char **argv)
{
    FILE *file = fopen("chunky.txt", "r");

    int lines = 0;

#if 1
    int fd = fileno(file);

    struct stat stat;
    if (fstat(fd, &stat) != 0) {
        return -1;
    }

    const char *m = (const char *) mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);

    const char *line = m;
    for (const char *pch = strchr(m, '\n'); pch != NULL; pch = strchr(pch + 1, '\n')) {
        
        size_t line_size = (size_t) (void *) (pch - line);

        //fwrite(line, sizeof(char), line_size, stdout);
        //fputs("\n", stdout);

        lines++;

        line = pch + 1;
    }


    munmap((void *) m, stat.st_size);
#else
    
    size_t nread, len = 0;
    char *line = NULL;

    while ((nread = getline(&line, &len, file)) != -1) {
        lines++;
    }
    if (line != NULL) {
        free(line);
    }

#endif
    printf("%d\n", lines);
    fclose(file);



#if 0
    struct vp_list arr;

    vp_list_init(&arr);
    
    vp_list_append(&arr, "<block 00>");
    vp_list_append(&arr, "<block 01>");
    vp_list_append(&arr, "<block 02>");
    vp_list_append(&arr, "<block 03>");
    vp_list_append(&arr, "<block 04>");
    vp_list_append(&arr, "<block 05>");
    vp_list_append(&arr, "<block 06>");
    vp_list_append(&arr, "<block 07>");
    vp_list_append(&arr, "<block 08>");
    vp_list_append(&arr, "<block 09>");
    vp_list_append(&arr, "<block 10>");
    vp_list_append(&arr, "<block 11>");
    vp_list_append(&arr, "<block 12>");
    vp_list_append(&arr, "<block 13>");
    vp_list_append(&arr, "<block 14>");
    vp_list_append(&arr, "<block 15>");
    vp_list_append(&arr, "<block 16>");
    vp_list_append(&arr, "<block 17>");
    vp_list_append(&arr, "<block 18>");
    vp_list_append(&arr, "<block 19>");
    vp_list_append(&arr, "<block 20>");
    vp_list_debug_print(&arr);

    vp_list_insert(&arr, 10, "<block (10)>");
    vp_list_insert(&arr, 0, "<block (0)>");
    vp_list_debug_print(&arr);

    vp_list_remove(&arr, 11);
    vp_list_remove(&arr, 0);

    vp_list_debug_print(&arr);

    while (arr.size != 0) {
        vp_list_remove(&arr, 0);
        vp_list_debug_print(&arr);
    }
    
    vp_list_debug_print(&arr);

    vp_list_free(&arr);
#endif

    return 0;
}
