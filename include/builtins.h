#ifndef BUILTINS_H
#define BUILTINS_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int hush_cd(char **args);
int hush_help(char **args);
int hush_exit(char **args);

extern char *builtin_str[];
extern int (*builtin_func[])(char **);

int hush_num_builtins();

#endif // BUILTINS_H
