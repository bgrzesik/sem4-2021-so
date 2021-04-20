
#include <stdio.h>

#ifndef __INTERPRETER_H__
#define __INTERPRETER_H__

enum token_type {
    /* Commnad or name */
    TOKEN_LITERAL,

    /* | */
    TOKEN_PIPE,

    /* = */
    TOKEN_EQUALS,

    /* \n or \0 */
    TOKEN_ENDLINE,
};

struct token {
    enum token_type type;
    char *literal;
    size_t len;
};


int
eval(FILE *fin);


#endif
