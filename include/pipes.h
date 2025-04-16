#ifndef PIPES_H
#define PIPES_H

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Check if a token is a pipe symbol
int is_pipe(char *token);

// Split a command line by pipe symbols
char ***split_by_pipe(char **args, int *num_commands);

// Execute a pipeline of commands
int execute_pipeline(char **args);

#endif // PIPES_H
