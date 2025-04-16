#ifndef ALIAS_H
#define ALIAS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define an alias structure
typedef struct {
    char *name;
    char *value;
} Alias;

// Initialize the alias system
void init_aliases(void);

// Clean up alias system
void save_aliases(void);

// Add a new alias
int add_alias(const char *name, const char *value);

// Remove an alias
int remove_alias(const char *name);

// Get the value of an alias
char *get_alias(const char *name);

// Expand aliases in a command line
char **expand_aliases(char **args);

// Builtin commands
int hush_alias(char **args);
int hush_unalias(char **args);

#endif
