

#ifndef __LIB__
#define __LIB__


#if defined(LIB_STATIC)

/* #define LIB_EXPORT static */
#define LIB_EXPORT
#define LIB_TRAMPOLINES

#elif defined(LIB_SHARED)

#define LIB_EXPORT extern
#define LIB_TRAMPOLINES

#elif defined(LIB_DYNAMIC)

#include <dlfcn.h>

#ifndef LIB_DYLIB
#define LIB_DYLIB "liblib.so"
#endif

void *_lib_dylib_handle;

#ifndef LIB_NO_PROTOTYPES
#define LIB_NO_PROTOTYPES
#endif

#define LIB_MODULE(name) _##name

#define LIB__FN_TABLE_ENTRY(ret, fn, args, call)                              \
    ret (*fn) args;

#define LIB__FN_DEF_TABLE()                                                   \
    struct MODULE {                                                           \
        MODULE_EXPORTS(LIB__FN_TABLE_ENTRY)                                   \
    };                                                                        \
                                                                              \
    struct MODULE MODULE;


#define LIB__TRAMPOLINE(ret, fn, args, call)                                  \
    static inline ret fn args {                                               \
        return (* MODULE .fn) call;                                           \
    }

#define LIB__TRAMPOLINES()                                                    \
    MODULE_EXPORTS(LIB__TRAMPOLINE)


#define LIB__LOAD_FN(ret, fn, args, call)                                     \
    MODULE .fn = dlsym(_lib_dylib_handle, #fn);


#define LIB__LOAD_FN_TABLE(module)                                            \
    static inline void lib_load##module(void) {                               \
        MODULE_EXPORTS(LIB__LOAD_FN)                                          \
    }
    

#define LIB_TRAMPOLINES(module)                                               \
    LIB__FN_DEF_TABLE()                                                       \
    LIB__LOAD_FN_TABLE(module)                                                \
    LIB__TRAMPOLINES()
    
#else

#error "Define one of LIB_STATIC, LIB_SHARED, LIB_DYNAMIC"

#endif
#endif
