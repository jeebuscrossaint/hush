#include "splitline.h"

char **hush_split_line(char *line)
{
        int bufsize = HUSH_TOK_BUFSIZE, position = 0;
        char **tokens = malloc(bufsize * sizeof(char *));
        char *token;

        if (!tokens) {
                fprintf(stderr, "hush: allocation error\n");
                exit(EXIT_FAILURE);
        }

        token = strtok(line, HUSH_TOK_DELIM);
        while (token != NULL) {
                tokens[position] = token;
                position++;

                if (position >= bufsize) {
                        bufsize += HUSH_TOK_BUFSIZE;
                        tokens = realloc(tokens, bufsize * sizeof(char *));
                        if (!tokens) {
                                fprintf(stderr, "hush: allocation error\n");
                                exit(EXIT_FAILURE);
                        }
                }

                token = strtok(NULL, HUSH_TOK_DELIM);
        }
        tokens[position] = NULL;
        return tokens;
}