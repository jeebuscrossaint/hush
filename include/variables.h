#ifndef VARIABLES_H
#define VARIABLES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

// Initialize shell variables
void init_shell_variables();

// Set a shell variable
int set_shell_variable(const char *name, const char *value);

// Get a shell variable value
char *get_shell_variable(const char *name);

// Unset a shell variable
int unset_shell_variable(const char *name);

// Set the last exit status
void set_last_exit_status(int status);

// Get the last exit status
int get_last_exit_status();

// Set last background process PID
void set_last_background_pid(pid_t pid);

// Get last background process PID
pid_t get_last_background_pid();

// Set command line arguments for scripts
void set_script_args(int argc, char **argv);

// Get count of script arguments
int get_script_arg_count();

// Get specific script argument
char *get_script_arg(int index);

// Expand variables in a string
char *expand_variables(char *line);

// Advanced parameter expansion like ${var:-default}
char *expand_parameter(const char *param, int *offset);

// Built-in 'set' command - display or set variables
int hush_set(char **args);

// Built-in 'unset' command
int hush_unset(char **args);

// Built-in 'shift' command - shift positional parameters
int hush_shift(char **args);

#endif // VARIABLES_H
