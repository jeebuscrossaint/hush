#include "loop.h"
#include "environment.h"
#include "jobs.h"
#include "signals.h"
#include "history.h"
#include "command_sub.h"
#include "alias.h"
#include "control.h"
#include "chain.h"
#include "splitline.h"
#include "readline.h"
#include "variables.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void hush_loop(void) {
    char *line;
    char **args;
    int status = 1;

    // For multi-line input (if statements, loops)
    char **script_lines = NULL;
    int script_line_count = 0;
    int in_control_block = 0;
    int control_nesting = 0;

    do {

        // Update status of any background jobs
        update_all_jobs_status();

        // Set proper prompt based on whether we're in a control block
        char *prompt = in_control_block ? "> " : "$ ";

        // Read input using readline
        line = hush_read_line();  // Use your existing readline function instead of direct readline call

        // Check for EOF
        if (!line) {
            printf("\n");

            // If in a control block, treat EOF as syntax error
            if (in_control_block) {
                fprintf(stderr, "hush: syntax error: unexpected end of file\n");

                // Clean up any collected script lines
                for (int i = 0; i < script_line_count; i++) {
                    free(script_lines[i]);
                }
                free(script_lines);
                script_lines = NULL;
                script_line_count = 0;
                in_control_block = 0;
            }

            break;
        }

        // Skip empty lines, but not in control blocks
        if (line[0] == '\0' && !in_control_block) {
            free(line);
            continue;
        }

        // Expand history references
        char *expanded_history = hush_expand_history(line);
        free(line);
        line = expanded_history;

        // Check for control structure keywords
        int keyword = is_control_keyword(line);
        int loop_start = is_loop_start(line);

        if (keyword == 1 || loop_start > 0) {  // "if" or loop start (for/while)
            // Start collecting a control block
            in_control_block = 1;
            control_nesting++;

            // Allocate or extend the script lines array
            if (script_line_count == 0) {
                script_lines = malloc(10 * sizeof(char *));
                if (!script_lines) {
                    perror("hush: memory allocation error");
                    free(line);
                    return;
                }
            } else {
                char **new_lines = realloc(script_lines, (script_line_count + 10) * sizeof(char *));
                if (!new_lines) {
                    perror("hush: memory allocation error");
                    for (int i = 0; i < script_line_count; i++) {
                        free(script_lines[i]);
                    }
                    free(script_lines);
                    free(line);
                    return;
                }
                script_lines = new_lines;
            }

            // Save this line
            script_lines[script_line_count++] = strdup(line);
        }
        else if (in_control_block) {
            // In a control block, keep collecting lines

            // Check for nested blocks or loops
            if (keyword == 1 || loop_start > 0) {
                control_nesting++;
            } else if (keyword == 5 || is_loop_end(line)) { // "fi" or "done"
                control_nesting--;
            }

            // Add this line to the script
            char **new_lines = realloc(script_lines, (script_line_count + 1) * sizeof(char *));
            if (!new_lines) {
                perror("hush: memory allocation error");
                for (int i = 0; i < script_line_count; i++) {
                    free(script_lines[i]);
                }
                free(script_lines);
                free(line);
                return;
            }
            script_lines = new_lines;
            script_lines[script_line_count++] = strdup(line);

            // If we've closed all if/loop blocks, execute the whole thing
            if (control_nesting == 0) {
                in_control_block = 0;

                // Perform variable expansion, command substitution, etc.
                for (int i = 0; i < script_line_count; i++) {
                    char *cmd_substituted = perform_command_substitution(script_lines[i]);
                    free(script_lines[i]);
                    script_lines[i] = cmd_substituted;

                    char *expanded_vars = expand_variables(script_lines[i]);
                    free(script_lines[i]);
                    script_lines[i] = expanded_vars;
                }

                // Execute the control block
                status = parse_and_execute_control(script_lines, script_line_count);

                // Clean up
                for (int i = 0; i < script_line_count; i++) {
                    free(script_lines[i]);
                }
                free(script_lines);
                script_lines = NULL;
                script_line_count = 0;
            }
        }
        else {
            // Regular command processing

            // Check if this is a loop keyword outside of a block
            if (loop_start > 0) {
                fprintf(stderr, "hush: syntax error: unexpected %s\n",
                        loop_start == 1 ? "for" : "while");
                free(line);
                continue;
            }

            // Perform command substitution
            char *cmd_substituted = perform_command_substitution(line);
            free(line);
            line = cmd_substituted;

            // Add to history - both our internal history and readline's
            hush_add_to_history(line);

            // Expand variables
            char *expanded_line = expand_variables(line);
            free(line);

            // Check for command chaining
            if (strstr(expanded_line, ";") || strstr(expanded_line, "&&") || strstr(expanded_line, "||")) {
                status = execute_command_chain(expanded_line);
            } else {
                args = hush_split_line(expanded_line);
                char **expanded_args = expand_aliases(args);
                status = hush_execute(expanded_args);

                // Clean up
                if (expanded_args != args) {
                    for (int i = 0; expanded_args[i] != NULL; i++) {
                        free(expanded_args[i]);
                    }
                    free(expanded_args);
                }
                free(args);
            }

            free(expanded_line);
        }

    } while (status);

    // Final cleanup in case of unexpected exit while in a control block
    if (script_lines) {
        for (int i = 0; i < script_line_count; i++) {
            free(script_lines[i]);
        }
        free(script_lines);
    }
}
