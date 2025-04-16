#ifndef HISTORY_H
#define HISTORY_H

#define HISTORY_MAX 100

char *history_list[HISTORY_MAX];
int history_count;

void add_to_history(char *line);
void save_history(void);
void load_history(void);
int hush_history(char **args); // Add "history" builtin

#endif // HISTORY_H
