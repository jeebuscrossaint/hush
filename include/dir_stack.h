#ifndef DIRSTACK_H
#define DIRSTACK_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

// Initialize the directory stack
void init_dir_stack(void);

// Clean up the directory stack
void free_dir_stack(void);

// Push a directory onto the stack (pushd command)
int hush_pushd(char **args);

// Pop a directory from the stack (popd command)
int hush_popd(char **args);

// Display the directory stack (dirs command)
int hush_dirs(char **args);

#endif // DIRSTACK_H
