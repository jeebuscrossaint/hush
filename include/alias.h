#ifndef ALIAS_H
#define ALIAS_H

#include <stdlib.h>
#include <string.h>

typedef struct {
    char *name;
    char *value;
} Alias;

int hush_alias(char **args);
int hush_unalias(char **args);
char **expand_aliases(char **args);

#endif // ALIAS_H
