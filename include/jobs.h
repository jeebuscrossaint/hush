#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

// Structure to track background jobs
typedef struct {
    pid_t pid;
    char *command;
    int status; // 0: running, 1: stopped, 2: done
} Job;

int put_job_in_background(char **args);
int list_jobs(char **args); // Add "jobs" builtin

#endif // JOBS_H
