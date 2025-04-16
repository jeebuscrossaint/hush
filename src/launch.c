#include "launch.h"
#include "signals.h"
#include "redirection.h"

// Declare the external variable
extern volatile sig_atomic_t child_running;

int hush_launch(char **args)
{
    pid_t pid, wpid;
    int status;

    // Create copies of the file descriptors in case we need redirection
    int stdin_copy = -1, stdout_copy = -1, stderr_copy = -1;

    // Setup redirection and get clean args (without redirection operators)
    char **clean_args = setup_redirection(args, &stdin_copy, &stdout_copy, &stderr_copy);

    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(clean_args[0], clean_args) == -1) {
            perror("hush");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        perror("hush");
    } else {
        // Parent process
        child_running = 1;  // Set the flag

        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));

        child_running = 0;  // Clear the flag
    }

    // Reset file descriptors to their original state
    reset_redirection(stdin_copy, stdout_copy, stderr_copy);

    // Free the clean_args array if it's different from args
    if (clean_args != args) {
        free(clean_args);
    }

    return 1;
}
