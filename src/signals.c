#include "signals.h"
#include "readline.h"
#include "jobs.h"  // Make sure this is included
#include <stdlib.h>
#include <errno.h>
#include <readline/readline.h>

// Define the global flag
volatile sig_atomic_t child_running = 0;

void handle_sigint(int sig) {
    if (!child_running) {
        // Only handle for the shell, not for child processes

        // Write a newline - suppress warning with cast
        (void)write(STDOUT_FILENO, "\n", 1);

        // Tell readline we're on a new line
        rl_on_new_line();

        // Clear the current input
        rl_replace_line("", 0);

        // Redisplay the prompt
        rl_redisplay();

        // Ensure our signal handler doesn't interfere with terminal
        signal(SIGINT, handle_sigint);
    }
}

void handle_sigtstp(int sig) {
    if (!child_running) {
        // Write a newline - suppress warning with cast
        (void)write(STDOUT_FILENO, "\n", 1);

        // Tell readline we're on a new line
        rl_on_new_line();

        // Clear the current input
        rl_replace_line("", 0);

        // Redisplay the prompt
        rl_redisplay();

        // Ensure our signal handler doesn't interfere with terminal
        signal(SIGTSTP, handle_sigtstp);
    }
}

void handle_sigwinch(int sig) {
    // Window size changed
    // We'll let the next refresh_line call handle it
    signal(SIGWINCH, handle_sigwinch);
}

void setup_signal_handlers(void) {
    // Use simple signal() for this basic implementation
    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);
    signal(SIGWINCH, handle_sigwinch);
    signal(SIGCHLD, handle_sigchld);  // Add SIGCHLD handler here
}

void handle_sigchld(int sig) {
    // Store the old errno to restore it after signal handling
    int saved_errno = errno;
    pid_t pid;
    int status;

    // Reap zombie processes
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        // Update process status
        update_process_status(pid, status);
    }

    // Restore errno
    errno = saved_errno;

    // Reset the signal handler
    signal(SIGCHLD, handle_sigchld);
}
