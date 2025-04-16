#ifndef COMMAND_SUB_H
#define COMMAND_SUB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// Perform command substitution on a line
char *perform_command_substitution(char *line);

// Check if a string contains command substitution
int has_command_substitution(const char *line);

// Execute a command and return its output as a string
char *capture_command_output(const char *command);

#endif // COMMAND_SUB_H
