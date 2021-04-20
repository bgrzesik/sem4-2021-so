#include <stdio.h>
#include <string.h>

#include "list.h"
#include "exec_chain.h"
#include "interpreter.h"


static int
is_token_end(char c) {
    return c == '\0' || c == '\n' || c == '|' || c == '=' || c == '#';
}

static void
next_token(struct token *token, char **tok_end)
{
    while (**tok_end == ' ' || **tok_end == '\t') {
        (*tok_end)++;
    }

    token->literal = *tok_end;
    token->len = 0;
    
    switch (**tok_end) {
        case '|':
            token->type = TOKEN_PIPE;
            token->len = 1;
            (*tok_end)++;
            return;

        case '=':
            token->type = TOKEN_EQUALS;
            token->len = 1;
            (*tok_end)++;
            return;

        case '#':
        case '\n':
        case '\0':
            token->type = TOKEN_ENDLINE;
            token->len = 1;
            (*tok_end)++;
            return;
    }

    token->type = TOKEN_LITERAL;
    while (!is_token_end(**tok_end)) {
        (*tok_end)++;
        token->len++;
    }

    while (token->len > 1 && *(token->literal + token->len - 1) == ' ') {
        token->len--;
    }
}


static int
read_pipe_list(struct token *last, struct token *cmd, char **cursor)
{
    next_token(cmd, cursor);

    if (cmd->type != TOKEN_LITERAL) {
        fprintf(stderr, "error: unexpected token\n");
        return 1;
    }

    next_token(last, cursor);

    if (last->type != TOKEN_ENDLINE && last->type != TOKEN_PIPE) {
        fprintf(stderr, "error: unexpected token\n");
        return 2;
    }

    return 0;
}

int
read_record(struct context *ctx, 
            struct token *first, struct token *token, 
            char **cursor)
{
    struct record record;
    record_init(&record, first);

    do {
        struct token cmd;
        int error = read_pipe_list(token, &cmd, cursor);

        if (error) {
            return error;
        }

        record_add_cmd(&record, &cmd);
    } while (token->type != TOKEN_ENDLINE);

    context_register(ctx, &record);

    return 0;
}

int
read_exec(struct context *ctx, 
          struct token *first, struct token *token, 
          char **cursor)
{
    context_push(ctx, first);

    if (token->type == TOKEN_PIPE) {
        struct token name;
        do {
            int error = read_pipe_list(token, &name, cursor);

            if (error) {
                return error;
            }

            error = context_push(ctx, &name);

            if (error) {
                return error;
            }
        } while (token->type != TOKEN_ENDLINE);
    }

    return context_exec(ctx);
}

int
eval(FILE *fin)
{
    char *line = NULL;
    size_t line_size = 0;
    int error = 0;

    struct context ctx;
    context_init(&ctx);

    struct list lines;
    list_init(&lines, sizeof(char *));

    while (!error && getline(&line, &line_size, fin) != -1) {
        struct token token;

        char *cursor = strdup(line);

        LIST_EMBRACE(&lines, char *, cursor);
    
        do {
            next_token(&token, &cursor);

            if (token.type == TOKEN_LITERAL) {
                struct token name = token;
                next_token(&token, &cursor);

                if (token.type == TOKEN_EQUALS) {
                    error = read_record(&ctx, &name, &token, &cursor);
                    break;

                } else if (token.type == TOKEN_PIPE || token.type == TOKEN_ENDLINE) {
                    error = read_exec(&ctx, &name, &token, &cursor);
                    break;

                } else {
                    fprintf(stderr, "error: unexpected token 1\n");
                    error = 1;
                    break;
                }

            } else if (token.type == TOKEN_ENDLINE) {
                break;

            } else {
                fprintf(stderr, "error: unexpected token\n");
                error = 1;
                break;
            }

        } while (token.type != TOKEN_ENDLINE && !error);
    }

    if (line != NULL) {
        free(line);
    }

    LIST_FOREACH(&lines, char *, line) {
        free(*line);
    }

    list_free(&lines);
    context_free(&ctx);

    return error;
}
