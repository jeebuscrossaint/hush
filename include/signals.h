#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>
#include <unistd.h>
#include <stdio.h>

// Global flag to track if a child process is running
extern volatile sig_atomic_t child_running;

// Flag to indicate a signal was received
extern volatile sig_atomic_t signal_received;

// Set up signal handlers for the shell
void setup_signal_handlers(void);

// Handler for SIGINT (Ctrl+C)
void handle_sigint(int sig);

// Handler for SIGTSTP (Ctrl+Z)
void handle_sigtstp(int sig);

// Handler for SIGCHLD (child process status change)
void handle_sigchld(int sig);

#endif // SIGNALS_H
