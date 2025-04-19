#ifndef VARIABLES_H
#define VARIABLES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Initialize shell variables
void init_shell_variables();

// Set a shell variable
int set_shell_variable(const char *name, const char *value);

// Get a shell variable value
char *get_shell_variable(const char *name);

// Unset a shell variable
int unset_shell_variable(const char *name);

// Expand variables in a string (already implemented in environment.c)
char *expand_variables(char *line);

// Built-in 'set' command - display or set variables
int hush_set(char **args);

// Built-in 'unset' command
int hush_unset(char **args);

#endif // VARIABLES_H
