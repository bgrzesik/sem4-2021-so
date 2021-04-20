

#include "list.h"
#include "interpreter.h"


#ifndef __EXEC_CHAIN_H__
#define __EXEC_CHAIN_H__

struct cmd {
    struct token cmd;
    int proc_stdin;
    int proc_stdout;
};

struct context {
    struct list records;
    struct list stack;
};

struct record {
    struct token name;
    struct list cmds;
};

void
context_init(struct context *ctx);

void
context_register(struct context *ctx, struct record *record);

int
context_push(struct context *ctx, struct token *name);

int
context_exec(struct context *ctx);

void
context_free(struct context *ctx);

void
record_init(struct record *record, struct token *name);

void
record_add_cmd(struct record *record, struct token *cmd);

void
record_free(struct record *record);


#endif
