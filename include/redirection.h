#ifndef REDIRECTION_H
#define REDIRECTION_H

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

// Check if a token is a redirection operator
int is_redirection(char *token);

// Setup IO redirection for a command
// Returns new args array with redirection operators removed
char **setup_redirection(char **args, int *stdin_copy, int *stdout_copy, int *stderr_copy);

// Reset IO after command completes
void reset_redirection(int stdin_copy, int stdout_copy, int stderr_copy);

#endif // REDIRECTION_H
