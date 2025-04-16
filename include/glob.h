#ifndef GLOB_H
#define GLOB_H

#include <glob.h>
#include <stdio.h>
#include <string.h>

// Check if a token contains wildcard characters
int has_wildcards(const char *token);

// Expand a single argument that might contain wildcards
char **expand_wildcard(char *arg, int *count);

// Expand all arguments that contain wildcards
char **expand_wildcards(char **args);

#endif // GLOB_H
