#ifndef READLINE_H
#define READLINE_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define HUSH_RL_BUFSIZE 1024

// Key definitions
#define CTRL_KEY(k) ((k) & 0x1f)
#define ARROW_UP 1000
#define ARROW_DOWN 1001
#define ARROW_LEFT 1002
#define ARROW_RIGHT 1003
#define HOME_KEY 1004
#define END_KEY 1005
#define DELETE_KEY 1006
#define BACKSPACE 127

// Terminal control functions
void enable_raw_mode();
void disable_raw_mode();
int read_key();
int get_cursor_position(int *rows, int *cols);
int get_window_size(int *rows, int *cols);
void refresh_line(char *buf, int pos, int len, int prompt_len);
char *hush_read_line(void);

#endif // READLINE_H
