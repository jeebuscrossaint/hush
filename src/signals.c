#include "signals.h"
#include <stdlib.h>
#include <errno.h>

// Define the global flag
volatile sig_atomic_t child_running = 0;

void handle_sigint(int sig) {
    if (!child_running) {
        // Only handle for the shell, not for child processes
        ssize_t ret = write(STDOUT_FILENO, "\n$ ", 3);
        (void)ret; // Suppress the warning

        // Ensure our signal handler doesn't interfere with getchar
        signal(SIGINT, handle_sigint);
    }
}

void handle_sigtstp(int sig) {
    if (!child_running) {
        // Only handle for the shell, not for child processes
        ssize_t ret = write(STDOUT_FILENO, "\n$ ", 3);
        (void)ret; // Suppress the warning

        // Ensure our signal handler doesn't interfere with getchar
        signal(SIGTSTP, handle_sigtstp);
    }
}

void setup_signal_handlers(void) {
    // Use simple signal() for this basic implementation
    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);
}
