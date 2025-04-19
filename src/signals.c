#include "signals.h"
#include "readline.h"
#include <stdlib.h>
#include <errno.h>
#include <readline/readline.h>

// Define the global flag
volatile sig_atomic_t child_running = 0;

void handle_sigint(int sig) {
    if (!child_running) {
        // Only handle for the shell, not for child processes

        // Write a newline
        write(STDOUT_FILENO, "\n", 1);

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
        // Write a newline
        write(STDOUT_FILENO, "\n", 1);

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
}
