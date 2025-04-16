#ifndef REDIRECTION_H
#define REDIRECTION_H

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int setup_redirection(char **args);

void reset_redirection(int stdin_copy, int stdout_copy);

#endif // REDIRECTION_H
