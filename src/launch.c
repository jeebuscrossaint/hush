#include "launch.h"
#include "signals.h"
#include "redirection.h"
#include "jobs.h"

// Declare the external variable
extern volatile sig_atomic_t child_running;

int hush_launch(char **args) {
    // Create a job for this command
    char command_str[1024] = {0};
    int i = 0;
    while (args[i]) {
        if (i > 0) strcat(command_str, " ");
        strcat(command_str, args[i]);
        i++;
    }

    // Check if command should run in background
    int run_in_background = 0;
    if (args[i-1] && strcmp(args[i-1], "&") == 0) {
        args[i-1] = NULL;  // Remove & from args
        run_in_background = 1;
    }

    // Create copies of the file descriptors in case we need redirection
    int stdin_copy = -1, stdout_copy = -1, stderr_copy = -1;

    // Setup redirection and get clean args (without redirection operators)
    char **clean_args = setup_redirection(args, &stdin_copy, &stdout_copy, &stderr_copy);

    // Create a new job
    Job *job = create_job(command_str);
    if (!job) {
        reset_redirection(stdin_copy, stdout_copy, stderr_copy);
        if (clean_args != args) free(clean_args);
        return 1;
    }

    // Launch the job (with proper job control)
    int status = launch_job(job, clean_args, !run_in_background);

    // Reset file descriptors to their original state
    reset_redirection(stdin_copy, stdout_copy, stderr_copy);

    // Free the clean_args array if it's different from args
    if (clean_args != args) {
        free(clean_args);
    }

    return status;
}
