#include "loop.h"
#include "environment.h"
#include "signals.h"

void hush_loop(void)
{
    char *line;
    char **args;
    int status;

    do {
        // Print the prompt
        printf("$ ");
        fflush(stdout);

        // Read input
        line = hush_read_line();

        // Empty line check
        if (line[0] == '\0') {
            free(line);
            continue;
        }

        // Process the line
        char *expanded_line = expand_variables(line);
        free(line);

        args = hush_split_line(expanded_line);
        status = hush_execute(args);

        free(expanded_line);
        free(args);
    } while (status);
}
