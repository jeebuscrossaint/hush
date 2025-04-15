#include "loop.h"

void hush_loop(void)
{
        char *line;
        char **args;
        int status;

        do {
                printf("> ");
                line = hush_read_line();
                args = hush_split_line(line);
                status = hush_execute(args);

                free(line);
                free(args);
        } while (status);
}