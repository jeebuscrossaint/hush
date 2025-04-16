#ifndef PIPES_H
#define PIPES_H

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int execute_pipeline(char **commands[], int num_commands);

#endif // PIPES_H
