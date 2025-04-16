#include "pipes.h"
#include "launch.h"
#include "redirection.h"
#include "signals.h"
#include <sys/wait.h>

// External variable
extern volatile sig_atomic_t child_running;

// Helper function to check if a token is a pipe
int is_pipe(char *token) {
    return (token != NULL && strcmp(token, "|") == 0);
}

// Split a command line by pipe symbols
char ***split_by_pipe(char **args, int *num_commands) {
    // First, count the number of commands (separated by pipes)
    int count = 1; // At least one command
    for (int i = 0; args[i] != NULL; i++) {
        if (is_pipe(args[i])) {
            count++;
        }
    }

    // Allocate space for the commands array
    char ***commands = malloc(count * sizeof(char **));
    if (!commands) {
        perror("hush: allocation error");
        exit(EXIT_FAILURE);
    }

    // Now split the args into separate commands
    int cmd_index = 0;
    int start_index = 0;

    for (int i = 0; args[i] != NULL; i++) {
        if (is_pipe(args[i]) || args[i+1] == NULL) {
            // Found a pipe or the end of args
            int arg_count = i - start_index;
            if (!is_pipe(args[i])) {
                arg_count++; // Include the last argument if not a pipe
            }

            // Allocate space for this command's arguments
            commands[cmd_index] = malloc((arg_count + 1) * sizeof(char *));
            if (!commands[cmd_index]) {
                perror("hush: allocation error");
                exit(EXIT_FAILURE);
            }

            // Copy the arguments for this command
            for (int j = 0; j < arg_count; j++) {
                commands[cmd_index][j] = strdup(args[start_index + j]);
            }
            commands[cmd_index][arg_count] = NULL; // Null-terminate

            cmd_index++;
            start_index = i + 1; // Skip past the pipe
        }
    }

    *num_commands = count;
    return commands;
}

// Execute a pipeline of commands
int execute_pipeline(char **args) {
    int num_commands = 0;
    char ***commands = split_by_pipe(args, &num_commands);

    if (num_commands == 1) {
        // No pipes, just execute normally
        int result = hush_launch(commands[0]);

        // Free memory allocated for commands
        for (int i = 0; commands[0][i] != NULL; i++) {
            free(commands[0][i]);
        }
        free(commands[0]);
        free(commands);

        return result;
    }

    // Array to hold all pipe file descriptors
    int pipes[num_commands-1][2];

    // Create all pipes
    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("hush: pipe error");
            // Clean up
            for (int j = 0; j < num_commands; j++) {
                for (int k = 0; commands[j][k] != NULL; k++) {
                    free(commands[j][k]);
                }
                free(commands[j]);
            }
            free(commands);
            return 1;
        }
    }

    // Prepare to store all child PIDs
    pid_t pids[num_commands];

    // Run all commands in the pipeline
    for (int i = 0; i < num_commands; i++) {
        pids[i] = fork();

        if (pids[i] < 0) {
            // Error forking
            perror("hush: fork error");
            exit(EXIT_FAILURE);
        } else if (pids[i] == 0) {
            // Child process

            // Set up stdin from the previous pipe (if not first command)
            if (i > 0) {
                if (dup2(pipes[i-1][0], STDIN_FILENO) == -1) {
                    perror("hush: dup2 error");
                    exit(EXIT_FAILURE);
                }
            }

            // Set up stdout to the next pipe (if not last command)
            if (i < num_commands - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("hush: dup2 error");
                    exit(EXIT_FAILURE);
                }
            }

            // Close all pipe file descriptors in the child
            for (int j = 0; j < num_commands - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Apply any redirection in the command
            int stdin_copy = -1, stdout_copy = -1, stderr_copy = -1;
            char **clean_args = setup_redirection(commands[i], &stdin_copy, &stdout_copy, &stderr_copy);

            // Execute the command
            if (execvp(clean_args[0], clean_args) == -1) {
                perror("hush");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Parent process closes all pipe file descriptors
    for (int i = 0; i < num_commands - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes to finish
    child_running = 1;
    int status;
    for (int i = 0; i < num_commands; i++) {
        do {
            waitpid(pids[i], &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    child_running = 0;

    // Clean up memory
    for (int i = 0; i < num_commands; i++) {
        for (int j = 0; commands[i][j] != NULL; j++) {
            free(commands[i][j]);
        }
        free(commands[i]);
    }
    free(commands);

    return 1;
}
