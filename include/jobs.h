#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

// Maximum number of jobs the shell can handle
#define MAX_JOBS 20

// Job states
typedef enum {
    JOB_RUNNING,    // Job is running
    JOB_STOPPED,    // Job is stopped (suspended)
    JOB_DONE,       // Job has completed
    JOB_TERMINATED  // Job was terminated by a signal
} JobState;

// Structure to represent a process in a job
typedef struct process {
    struct process *next;    // Next process in the job
    pid_t pid;               // Process ID
    int completed;           // True if process has completed
    int stopped;             // True if process has stopped
    int status;              // Exit status or termination signal
} Process;

// Structure to represent a job
typedef struct job {
    int id;                  // Job ID [1, 2, 3, ...]
    Process *first_process;  // Linked list of processes in this job
    pid_t pgid;              // Process group ID
    char *command;           // Command line used to start the job
    JobState state;          // Current state of the job
    struct termios tmodes;   // Terminal modes
    int notified;            // True if user was notified about state change
    int foreground;          // Is job running in foreground?
} Job;

// Terminal information
extern pid_t shell_pgid;
extern int shell_terminal;
extern int shell_is_interactive;
extern struct termios shell_tmodes;

// Current active jobs array
extern Job *jobs[MAX_JOBS];

// Initialize job control
void init_job_control(void);

// Find an empty slot in jobs array for a new job
int find_empty_job_slot(void);

// Allocate a new job
Job *create_job(char *command);

// Add a process to a job
void add_process_to_job(Job *job, pid_t pid);

// Find a job by its ID
Job *find_job_by_id(int id);

// Find a job by its process group ID
Job *find_job_by_pgid(pid_t pgid);

// Check if all processes in job are completed
int job_is_completed(Job *job);

// Check if all processes in job are stopped
int job_is_stopped(Job *job);

// Update the status of all processes in a job
void update_job_status(Job *job, pid_t pid, int status);

// Update the status of all jobs
void update_all_jobs_status(void);

// Wait for a specific job to change state
void wait_for_job(Job *job);

// Mark a job as running in foreground
void put_job_in_foreground(Job *job, int cont);

// Mark a job as running in background
void put_job_in_background(Job *job, int cont);

// Report change in job status to user
void format_job_info(Job *job, const char *status);

// Free all resources associated with a job
void free_job(Job *job);

// Remove a completed job from jobs list
void remove_job(int job_id);

// Continue a job in foreground/background
void continue_job(Job *job, int foreground);

// Launch a job (for both foreground and background processes)
int launch_job(Job *job, char **args, int foreground);

// Update the status of a specific process
void update_process_status(pid_t pid, int status);

// Built-in commands
int hush_jobs(char **args);
int hush_fg(char **args);
int hush_bg(char **args);
int hush_wait(char **args);
int hush_disown(char **args);

#endif // JOBS_H
