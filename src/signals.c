#include "signals.h"
#include <stdlib.h>

// Define the global flag
volatile sig_atomic_t child_running = 0;

void handle_sigint(int sig) {
    // Write a newline to go after the ^C and handle the return value
    ssize_t ret = write(STDOUT_FILENO, "\n", 1);
    (void)ret; // Suppress unused result warning

    // If no child is running, redisplay the prompt
    if (!child_running) {
        ret = write(STDOUT_FILENO, "$ ", 2);
        (void)ret; // Suppress unused result warning
    }

    // We don't exit the shell, just let the signal handler return
}

void handle_sigtstp(int sig) {
    // Handle Ctrl+Z (SIGTSTP)
    // Similar behavior to SIGINT for now
    ssize_t ret = write(STDOUT_FILENO, "\n", 1);
    (void)ret; // Suppress unused result warning

    if (!child_running) {
        ret = write(STDOUT_FILENO, "$ ", 2);
        (void)ret; // Suppress unused result warning
    }
}

void setup_signal_handlers(void) {
    // Set up signal handlers
    struct sigaction sa;

    // Initialize the sigaction struct
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    // Set handler for SIGINT (Ctrl+C)
    sa.sa_handler = handle_sigint;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction for SIGINT");
        exit(EXIT_FAILURE);
    }

    // Set handler for SIGTSTP (Ctrl+Z)
    sa.sa_handler = handle_sigtstp;
    if (sigaction(SIGTSTP, &sa, NULL) == -1) {
        perror("sigaction for SIGTSTP");
        exit(EXIT_FAILURE);
    }
}
