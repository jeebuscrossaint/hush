#include "splitline.h"

char **hush_split_line(char *line)
{
    int bufsize = HUSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));

    if (!tokens) {
        fprintf(stderr, "hush: allocation error\n");
        exit(EXIT_FAILURE);
    }

    // Preprocess to add spaces around pipe symbols
    char *new_line = malloc(strlen(line) * 3 + 1);
    if (!new_line) {
        fprintf(stderr, "hush: allocation error\n");
        exit(EXIT_FAILURE);
    }

    int j = 0;
    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == '|') {
            // Add space before pipe
            new_line[j++] = ' ';
            // Add pipe
            new_line[j++] = '|';
            // Add space after pipe
            new_line[j++] = ' ';
        } else {
            new_line[j++] = line[i];
        }
    }
    new_line[j] = '\0';

    // Use our own parsing that handles quoted strings
    int i = 0;
    int len = strlen(new_line);
    char buffer[1024]; // Buffer for current token
    int in_quotes = 0; // 0 = not in quotes, 1 = single quotes, 2 = double quotes
    int buf_pos = 0;

    while (i <= len) { // Include the null terminator in the loop
        char c = new_line[i];

        // Handle end of string or whitespace outside quotes
        if (c == '\0' || (c == ' ' || c == '\t' || c == '\n' || c == '\r') && !in_quotes) {
            if (buf_pos > 0) {
                // We have a token to add
                buffer[buf_pos] = '\0';
                tokens[position] = strdup(buffer);
                position++;

                if (position >= bufsize) {
                    bufsize += HUSH_TOK_BUFSIZE;
                    tokens = realloc(tokens, bufsize * sizeof(char *));
                    if (!tokens) {
                        fprintf(stderr, "hush: allocation error\n");
                        exit(EXIT_FAILURE);
                    }
                }

                // Reset buffer
                buf_pos = 0;
            }
        }
        // Handle quotes
        else if (c == '\'' && in_quotes != 2) { // Single quote and not in double quotes
            if (in_quotes == 1) {
                in_quotes = 0; // Exit single quotes
            } else {
                in_quotes = 1; // Enter single quotes
            }
        }
        else if (c == '\"' && in_quotes != 1) { // Double quote and not in single quotes
            if (in_quotes == 2) {
                in_quotes = 0; // Exit double quotes
            } else {
                in_quotes = 2; // Enter double quotes
            }
        }
        // Regular character - add to current token
        else {
            buffer[buf_pos++] = c;
        }

        i++;
    }

    tokens[position] = NULL;
    free(new_line);

    return tokens;
}
