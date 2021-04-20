#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

#include "exec_chain.h"
#include "list.h"


void
context_init(struct context *ctx)
{
    list_init(&ctx->records, sizeof(struct record));
    list_init(&ctx->stack, sizeof(struct record *));
}

int
context_push(struct context *ctx, struct token *name)
{
    struct record *found = NULL;

    LIST_FOREACH(&ctx->records, struct record, elem) {
        if (elem->name.len != name->len) {
            continue;
        }

        if (strncmp(elem->name.literal, name->literal, name->len) == 0) {
            found = elem;
            break;
        }
    }

    if (found == NULL) {
        return -1;
    }

    LIST_EMBRACE(&ctx->stack, struct record *, found);

    return 0;
}

void
context_register(struct context *ctx, struct record *record)
{
    LIST_EMBRACE(&ctx->records, struct record, *record);
}


static struct cmd *
context_get_first(struct context *ctx)
{
    struct record *record = * (struct record **) list_get(&ctx->stack, 0);

    struct cmd *first = (struct cmd *) list_get(&record->cmds, 0);

    return first;
}

static struct cmd *
context_get_last(struct context *ctx)
{
    struct record *record = * (struct record **) list_last(&ctx->stack);
    struct cmd *cmd = (struct cmd *) list_last(&record->cmds);
    return cmd;
}

static void
context_start_child(struct cmd *cmd, int proc_stdin, int proc_stdout)
{
    cmd->cmd.literal[cmd->cmd.len] = 0;

    struct list argv;
    list_init(&argv, sizeof(char *));

    char *arg = cmd->cmd.literal;
    char *cur = arg;

    while ((cur = strchr(arg, ' ')) != NULL) {
        cur[0] = 0;
        LIST_EMBRACE(&argv, char *, arg);
        arg = cur + 1;
    }

    if (*arg != 0) {
        LIST_EMBRACE(&argv, char *, arg);
    }

    LIST_EMBRACE(&argv, char *, NULL);

    if (proc_stdout != STDOUT_FILENO) {
        dup2(proc_stdout, STDOUT_FILENO);
    }
    if (proc_stdin != STDIN_FILENO) {
        dup2(proc_stdin, STDIN_FILENO);
    }


    execvp(*(char **) list_get(&argv, 0), (char **) argv.data);
    exit(-1);
}


int
context_exec(struct context *ctx)
{
    struct cmd *first = context_get_first(ctx);
    struct cmd *last = context_get_last(ctx);

    int proc_stdout = STDOUT_FILENO;

    LIST_FOREACH_REV(&ctx->stack, struct record *, precord) {
        struct record *record = *precord;

        LIST_FOREACH_REV(&record->cmds, struct cmd, cmd) {
            int fd[2];

            if (cmd != first) {
                if (pipe(fd) != 0) {
                    perror("pipe");
                    return -1;
                }
            }

            if (fork() == 0) {
                if (cmd != first) {
                    close(fd[1]);
                }
                context_start_child(cmd, fd[0], proc_stdout);
            }

            if (cmd != first) {
                close(fd[0]);
            }
            if (cmd != last) {
                close(proc_stdout);
            }
            proc_stdout = fd[1];
        }
    }

    pid_t pid;
    int ret;
    while ((pid = wait(&ret)) > 0) {
        if (!WIFEXITED(ret) || WEXITSTATUS(ret) != 0) {
            fprintf(stderr, "error: child %d returned %d\n", pid, WEXITSTATUS(ret));
            return -1;
        }
    }

    list_free(&ctx->stack);
    list_init(&ctx->stack, sizeof(struct record *));

    return 0;
}

void
context_free(struct context *ctx)
{
    LIST_FOREACH(&ctx->records, struct record, elem) {
        record_free(elem);
    }
    list_free(&ctx->records);
    list_free(&ctx->stack);
}

void
record_init(struct record *record, struct token *name) 
{
    record->name = *name;
    list_init(&record->cmds, sizeof(struct cmd));
}

void
record_add_cmd(struct record *record, struct token *token) 
{
    struct cmd *cmd = (struct cmd *) list_append(&record->cmds);

    cmd->cmd = *token;
    cmd->proc_stdin = STDIN_FILENO;
    cmd->proc_stdout = STDOUT_FILENO;
}

void
record_free(struct record *record)
{
    list_free(&record->cmds);
}
