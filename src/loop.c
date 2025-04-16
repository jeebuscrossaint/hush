#include "loop.h"
#include "environment.h"

void hush_loop(void)
{
    char *line;
    char **args;
    int status;

    do {
        printf("$ ");
        line = hush_read_line();

        // Expand environment variables in the input line
        char *expanded_line = expand_variables(line);
        free(line); // Free the original line

        args = hush_split_line(expanded_line);
        status = hush_execute(args);

        free(expanded_line);
        free(args);
    } while (status);
}
