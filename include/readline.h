#ifndef READLINE_H
#define READLINE_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define HUSH_RL_BUFSIZE 1024

char *hush_read_line(void);

#endif // READLINE_H