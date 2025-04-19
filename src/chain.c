#include "chain.h"
#include "execute.h"
#include "splitline.h"
#include "alias.h"
#include <ctype.h>

// Helper function to trim whitespace
static char *trim(char *str) {
    if (!str) return NULL;

    // Trim leading space
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0)  // All spaces
        return str;

    // Trim trailing space
    char *end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end + 1) = 0;

    return str;
}

// Parse and execute command chains (cmd1 && cmd2 || cmd3 ; cmd4)
int execute_command_chain(char *line) {
    int last_result = 1; // Success by default

    // Make a copy since we'll be modifying the string
    char *line_copy = strdup(line);
    if (!line_copy) {
        perror("hush: memory allocation error");
        return 1;
    }

    // First, split by semicolons
    char **commands = malloc(256 * sizeof(char*));
    if (!commands) {
        perror("hush: memory allocation error");
        free(line_copy);
        return 1;
    }

    int cmd_count = 0;
    char *saveptr;
    char *token = strtok_r(line_copy, ";", &saveptr);

    while (token != NULL && cmd_count < 255) {
        commands[cmd_count++] = strdup(token);
        token = strtok_r(NULL, ";", &saveptr);
    }
    commands[cmd_count] = NULL; // NULL terminate the array

    // We can free line_copy now since we've extracted all commands
    free(line_copy);

    // Process each command or command group
    for (int i = 0; i < cmd_count; i++) {
        char *cmd = trim(commands[i]);

        // Check for && or ||
        if (strstr(cmd, "&&") || strstr(cmd, "||")) {
            // We need to process operators
            char *cmd_copy = strdup(cmd);
            if (!cmd_copy) {
                perror("hush: memory allocation error");
                // Free all commands
                for (int j = 0; j < cmd_count; j++) {
                    free(commands[j]);
                }
                free(commands);
                return 1;
            }

            // Process the command with operators
            char *curr_pos = cmd_copy;
            char *next_and, *next_or, *next_op;
            int should_execute = 1;

            while (*curr_pos) {
                next_and = strstr(curr_pos, "&&");
                next_or = strstr(curr_pos, "||");

                // Determine which operator comes first
                if (next_and && next_or) {
                    next_op = (next_and < next_or) ? next_and : next_or;
                } else {
                    next_op = next_and ? next_and : next_or;
                }

                char op_type = 0;
                char *cmd_part;

                if (next_op) {
                    // Get operator type
                    op_type = (next_op == next_and) ? '&' : '|';

                    // Extract the command part
                    *next_op = '\0';
                    cmd_part = trim(curr_pos);

                    // Move the pointer past the operator
                    curr_pos = next_op + 2;
                } else {
                    // Last command in the chain
                    cmd_part = trim(curr_pos);
                    curr_pos += strlen(curr_pos);
                }

                // Decide if we should execute this part
                if (should_execute) {
                    if (strlen(cmd_part) > 0) {
                        char **args = hush_split_line(cmd_part);
                        char **expanded_args = expand_aliases(args);

                        last_result = hush_execute(expanded_args);

                        // Clean up
                        if (expanded_args != args) {
                            for (int j = 0; expanded_args[j]; j++) {
                                free(expanded_args[j]);
                            }
                            free(expanded_args);
                        }

                        for (int j = 0; args[j]; j++) {
                            free(args[j]);
                        }
                        free(args);
                    }
                }

                // Determine if we should execute the next part
                if (op_type == '&') {
                    should_execute = last_result; // Execute next only if this succeeded
                } else if (op_type == '|') {
                    should_execute = !last_result; // Execute next only if this failed
                }

                // If we're at the end, break
                if (!*curr_pos) break;
            }

            free(cmd_copy);

        } else {
            // Simple command without && or ||
            if (strlen(cmd) > 0) {
                char **args = hush_split_line(cmd);
                char **expanded_args = expand_aliases(args);

                last_result = hush_execute(expanded_args);

                // Clean up
                if (expanded_args != args) {
                    for (int j = 0; expanded_args[j]; j++) {
                        free(expanded_args[j]);
                    }
                    free(expanded_args);
                }

                for (int j = 0; args[j]; j++) {
                    free(args[j]);
                }
                free(args);
            }
        }
    }

    // Clean up all the commands
    for (int i = 0; i < cmd_count; i++) {
        free(commands[i]);
    }
    free(commands);

    return last_result;
}
