#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *expand_variables(char *line);
int hush_export(char **args); // Add "export" builtin

#endif // ENVIRONMENT_H
