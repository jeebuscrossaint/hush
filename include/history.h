#ifndef HISTORY_H
#define HISTORY_H

#include <errno.h>
#include <ctype.h>

#define HISTORY_MAX 100

// The history list (renamed to avoid conflict with readline)
extern char *hush_history_list[HISTORY_MAX];
extern int hush_history_count;

// Add a command to the history
void hush_add_to_history(char *line);

// Save history to file
void hush_save_history(void);

// Load history from file
void hush_load_history(void);

// Display history (builtin command)
int hush_history(char **args);

// Get history entry by number
char *hush_get_history_entry(int index);

// Expand history references (e.g., !!, !42, !-3)
char *hush_expand_history(char *line);

#endif // HISTORY_H
