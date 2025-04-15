#ifndef LAUNCH_H
#define LAUNCH_H

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

int hush_launch(char **args);

#endif // LAUNCH_H