#include "loop.h"
#include "environment.h"
#include "signals.h"
#include "history.h"
#include "command_sub.h"
#include "alias.h"

void hush_loop(void) {
    char *line;
    char **args;
    int status;

    do {
        // Read input (readline function now handles prompt display)
        line = hush_read_line();

        // Empty line check
        if (line[0] == '\0') {
            free(line);
            continue;
        }

        // Expand history references
        char *expanded_history = expand_history(line);
        free(line);
        line = expanded_history;

        // Perform command substitution
        char *cmd_substituted = perform_command_substitution(line);
        free(line);
        line = cmd_substituted;

        // Add to history
        add_to_history(line);

        // Process the line
        char *expanded_line = expand_variables(line);
        free(line);

        args = hush_split_line(expanded_line);

        // Expand aliases (if needed - this may already be handled in execute.c)
        char **expanded_args = expand_aliases(args);
        status = hush_execute(expanded_args);

        // Clean up - only free expanded_args if it's different from args
        if (expanded_args != args) {
            for (int i = 0; expanded_args[i] != NULL; i++) {
                free(expanded_args[i]);
            }
            free(expanded_args);
        }

        free(expanded_line);
        free(args);
    } while (status);
}
