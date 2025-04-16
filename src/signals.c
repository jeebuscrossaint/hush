#include "signals.h"
#include "readline.h"
#include <stdlib.h>
#include <errno.h>

// Define the global flag
volatile sig_atomic_t child_running = 0;

void handle_sigint(int sig) {
    if (!child_running) {
        // Only handle for the shell, not for child processes
        ssize_t ret = write(STDOUT_FILENO, "\n$ ", 3);
        (void)ret; // Suppress the warning

        // Ensure our signal handler doesn't interfere with terminal
        signal(SIGINT, handle_sigint);
    }
}

void handle_sigtstp(int sig) {
    if (!child_running) {
        // Only handle for the shell, not for child processes
        ssize_t ret = write(STDOUT_FILENO, "\n$ ", 3);
        (void)ret; // Suppress the warning

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
}
