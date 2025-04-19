#ifndef CONTROL_H
#define CONTROL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Check if line contains a control structure keyword (if, then, else, fi, etc.)
int is_control_keyword(const char *line);

// Execute an if-then-else-fi control structure
int execute_if_statement(char **lines, int line_count);

// Parse input for control structures and execute them
int parse_and_execute_control(char **lines, int line_count);

// Process control flow in a script
int process_script_control_flow(FILE *script_file);

// Execute a for loop
int execute_for_loop(char **lines, int line_count);

// Execute a while loop
int execute_while_loop(char **lines, int line_count);

// Check if a line starts a loop (for, while)
int is_loop_start(const char *line);

// Check if a line ends a loop (done)
int is_loop_end(const char *line);

#endif // CONTROL_H
