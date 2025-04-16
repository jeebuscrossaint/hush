#include "loop.h"
#include "environment.h"
#include "signals.h"
#include "history.h"
#include "command_sub.h"

void hush_loop(void)
{
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
        status = hush_execute(args);

        free(expanded_line);
        free(args);
    } while (status);
}
