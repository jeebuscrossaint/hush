#ifndef READLINE_H
#define READLINE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

// Initialize the readline library
void hush_readline_init(void);

// Read a line using readline
char *hush_read_line(void);

// Clean up readline
void hush_readline_cleanup(void);

#endif // READLINE_H
