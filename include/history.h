#ifndef HISTORY_H
#define HISTORY_H

#include <errno.h>
#include <ctype.h>

#define HISTORY_MAX 100

// The history list
extern char *history_list[HISTORY_MAX];
extern int history_count;

// Add a command to the history
void add_to_history(char *line);

// Save history to file
void save_history(void);

// Load history from file
void load_history(void);

// Display history (builtin command)
int hush_history(char **args);

// Get history entry by number
char *get_history_entry(int index);

// Expand history references (e.g., !!, !42, !-3)
char *expand_history(char *line);

#endif // HISTORY_H
