#ifndef SPLITLINE_H
#define SPLITLINE_H

#define HUSH_TOK_BUFSIZE 64
#define HUSH_TOK_DELIM " \t\r\n\a|"  // Add pipe to delimiters

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char **hush_split_line(char *line);

#endif // SPLITLINE_H
