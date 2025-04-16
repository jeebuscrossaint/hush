#include "execute.h"
#include "redirection.h"
#include "pipes.h"
#include "glob.h"

#include <sys/stat.h>
#include <limits.h>

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

    // Check if the first argument is a directory
    if (args[1] == NULL) {  // Only do this for single-argument commands
        struct stat st;

        // Handle tilde expansion for home directory
        char *path = args[0];
        char *expanded_path = NULL;

        if (path[0] == '~' && (path[1] == '/' || path[1] == '\0')) {
            const char *home = getenv("HOME");
            if (home) {
                expanded_path = malloc(strlen(home) + strlen(path));
                if (expanded_path) {
                    strcpy(expanded_path, home);
                    strcat(expanded_path, path + 1);  // Skip the ~
                    path = expanded_path;
                }
            }
        }

        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            // It's a directory, change to it (like implicit cd)
            if (chdir(path) != 0) {
                perror("hush");
            }

            // Print the new directory (like how fish does it)
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            }

            if (expanded_path) {
                free(expanded_path);
            }
            return 1; // Continue the shell loop
        }

        if (expanded_path) {
            free(expanded_path);
        }
    }

    // First, expand any wildcards in arguments
    char **expanded_args = expand_wildcards(args);


    // Check if the command contains pipes
    if (has_pipe(expanded_args)) {
        result = execute_pipeline(expanded_args);

        // Free expanded args
        for (i = 0; expanded_args[i] != NULL; i++) {
            free(expanded_args[i]);
        }
        free(expanded_args);

        return result;
    }

    // No pipes, proceed with normal execution
    // Create copies of the file descriptors for redirection
    int stdin_copy = -1, stdout_copy = -1, stderr_copy = -1;

    // Setup redirection and get clean args
    char **clean_args = setup_redirection(expanded_args, &stdin_copy, &stdout_copy, &stderr_copy);

    // Check for builtins
    for (i = 0; i < hush_num_builtins(); i++)
    {
        if (strcmp(clean_args[0], builtin_str[i]) == 0)
        {
            result = (*builtin_func[i])(clean_args);
            reset_redirection(stdin_copy, stdout_copy, stderr_copy);

            // Free the clean_args array if it's different from expanded_args
            if (clean_args != expanded_args) {
                free(clean_args);
            }

            // Free the expanded args
            for (i = 0; expanded_args[i] != NULL; i++) {
                free(expanded_args[i]);
            }
            free(expanded_args);

            return result;
        }
    }

    // Reset redirection since we're about to launch a new process
    reset_redirection(stdin_copy, stdout_copy, stderr_copy);

    // Free the clean_args if we created a new array
    if (clean_args != expanded_args) {
        free(clean_args);
    }

    // If not a builtin, launch the program with expanded args
    result = hush_launch(expanded_args);

    // Free the expanded args
    for (i = 0; expanded_args[i] != NULL; i++) {
        free(expanded_args[i]);
    }
    free(expanded_args);

    return result;
}
