#include "execute.h"
#include "redirection.h"
#include "pipes.h"

// Function to check if command contains a pipe
int has_pipe(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            return 1;
        }
    }
    return 0;
}

int hush_execute(char **args)
{
    int i;
    int result;

    if (args[0] == NULL)
    {
        return 1;
    }

    // Check if the command contains pipes
    if (has_pipe(args)) {
        return execute_pipeline(args);
    }

    // No pipes, proceed with normal execution
    // Create copies of the file descriptors for redirection
    int stdin_copy = -1, stdout_copy = -1, stderr_copy = -1;

    // Setup redirection and get clean args
    char **clean_args = setup_redirection(args, &stdin_copy, &stdout_copy, &stderr_copy);

    // Check for builtins
    for (i = 0; i < hush_num_builtins(); i++)
    {
        if (strcmp(clean_args[0], builtin_str[i]) == 0)
        {
            result = (*builtin_func[i])(clean_args);
            reset_redirection(stdin_copy, stdout_copy, stderr_copy);

            // Free the clean_args array if it's different from args
            if (clean_args != args) {
                free(clean_args);
            }

            return result;
        }
    }

    // Reset redirection since we're about to launch a new process
    reset_redirection(stdin_copy, stdout_copy, stderr_copy);

    // Free the clean_args if we created a new array
    if (clean_args != args) {
        free(clean_args);
    }

    // If not a builtin, launch the program
    return hush_launch(args);
}
