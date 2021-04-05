
#include <stdio.h>
#include <stdio.h>

#include "lib.h"

#ifndef __MERGE_H__
#define __MERGE_H__


#ifndef LIB_DYNAMIC

FILE *merge_files(FILE *left, FILE *right);

#else

#define MODULE LIB_MODULE(merge)
#define MODULE_EXPORTS(fn) \
    fn(FILE *, merge_files, (FILE *left, FILE* right), (left, right))

LIB_TRAMPOLINES(MODULE)

#undef MODULE_EXPORTS
#undef MODULE

#endif

#endif

