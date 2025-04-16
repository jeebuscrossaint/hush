#include "launch.h"
#include "signals.h"

// Declare the external variable
extern volatile sig_atomic_t child_running;

int hush_launch(char **args)
{
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
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
    return 1;
}
