#include "jobs.h"
#include "builtins.h"
#include "signals.h"
#include "readline.h"

// Terminal information
pid_t shell_pgid;
int shell_terminal;
int shell_is_interactive;
struct termios shell_tmodes;

// Current active jobs array
Job *jobs[MAX_JOBS] = {NULL};

// Initialize job control
void init_job_control(void) {
    // Check if we're running interactively
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty(shell_terminal);

    if (shell_is_interactive) {
        // Loop until we're in the foreground
        while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
            kill(-shell_pgid, SIGTTIN);

        // Ignore interactive and job-control signals
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGCHLD, handle_sigchld);

        // Put ourselves in our own process group
        shell_pgid = getpid();
        if (setpgid(shell_pgid, shell_pgid) < 0) {
            perror("hush: Couldn't put the shell in its own process group");
            exit(EXIT_FAILURE);
        }

        // Grab control of the terminal
        tcsetpgrp(shell_terminal, shell_pgid);

        // Save default terminal attributes
        tcgetattr(shell_terminal, &shell_tmodes);
    }

    // Initialize the jobs array
    for (int i = 0; i < MAX_JOBS; i++) {
        jobs[i] = NULL;
    }
}

// Find an empty slot in jobs array for a new job
int find_empty_job_slot(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i] == NULL) {
            return i;
        }
    }
    return -1; // No empty slots
}

// Allocate a new job
Job *create_job(char *command) {
    int job_slot = find_empty_job_slot();
    if (job_slot < 0) {
        fprintf(stderr, "hush: Too many jobs\n");
        return NULL;
    }

    Job *job = malloc(sizeof(Job));
    if (!job) {
        perror("hush: malloc error");
        return NULL;
    }

    job->id = job_slot + 1;  // Job IDs are 1-based
    job->first_process = NULL;
    job->pgid = 0;
    job->command = strdup(command);
    job->state = JOB_RUNNING;
    job->notified = 0;
    job->foreground = 1;  // Default to foreground

    jobs[job_slot] = job;
    return job;
}

// Add a process to a job
void add_process_to_job(Job *job, pid_t pid) {
    Process *proc = malloc(sizeof(Process));
    if (!proc) {
        perror("hush: malloc error");
        return;
    }

    proc->pid = pid;
    proc->completed = 0;
    proc->stopped = 0;
    proc->status = 0;
    proc->next = job->first_process;
    job->first_process = proc;

    // Set process group for the job if not already set
    if (job->pgid == 0) {
        job->pgid = pid;
    }
}

// Find a job by its ID
Job *find_job_by_id(int id) {
    if (id <= 0 || id > MAX_JOBS) {
        return NULL;
    }
    return jobs[id - 1];
}

// Find a job by its process group ID
Job *find_job_by_pgid(pid_t pgid) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i] && jobs[i]->pgid == pgid) {
            return jobs[i];
        }
    }
    return NULL;
}

// Check if all processes in job are completed
int job_is_completed(Job *job) {
    Process *p;

    for (p = job->first_process; p; p = p->next) {
        if (!p->completed) {
            return 0;
        }
    }
    return 1;
}

// Check if all processes in job are stopped
int job_is_stopped(Job *job) {
    Process *p;

    for (p = job->first_process; p; p = p->next) {
        if (!p->completed && !p->stopped) {
            return 0;
        }
    }
    return 1;
}

// Update the status of a process
void update_process_status(pid_t pid, int status) {
    // Find the job containing this process
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i]) {
            Process *p;
            for (p = jobs[i]->first_process; p; p = p->next) {
                if (p->pid == pid) {
                    p->status = status;
                    if (WIFSTOPPED(status)) {
                        p->stopped = 1;
                    } else {
                        p->completed = 1;
                        if (WIFSIGNALED(status)) {
                            fprintf(stderr, "\n%d: Terminated by signal %d\n",
                                    (int)pid, WTERMSIG(status));
                        }
                    }
                    return;
                }
            }
        }
    }
}

// Update status of all jobs
void update_all_jobs_status(void) {
    pid_t pid;
    int status;

    do {
        pid = waitpid(WAIT_ANY, &status, WUNTRACED | WNOHANG);
        if (pid > 0) {
            update_process_status(pid, status);
        }
    } while (pid > 0);

    // Update job states
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i]) {
            if (job_is_completed(jobs[i])) {
                if (!jobs[i]->notified) {
                    format_job_info(jobs[i], "Done");
                    jobs[i]->state = JOB_DONE;
                    jobs[i]->notified = 1;
                }
            } else if (job_is_stopped(jobs[i]) && jobs[i]->state != JOB_STOPPED) {
                format_job_info(jobs[i], "Stopped");
                jobs[i]->state = JOB_STOPPED;
                jobs[i]->notified = 1;
            }
        }
    }
}

// Wait for a specific job to change state
void wait_for_job(Job *job) {
    pid_t pid;
    int status;

    do {
        pid = waitpid(-job->pgid, &status, WUNTRACED);
        if (pid < 0) {
            if (errno == ECHILD) {
                // No more children to wait for
                break;
            } else if (errno != EINTR) {
                perror("hush: waitpid");
                break;
            }
        } else if (pid > 0) {
            update_process_status(pid, status);
        }
    } while (!job_is_stopped(job) && !job_is_completed(job));
}

// Put a job in the foreground
void put_job_in_foreground(Job *job, int cont) {
    // Put the job in the foreground
    tcsetpgrp(shell_terminal, job->pgid);

    // Send the job a continue signal if necessary
    if (cont) {
        if (kill(-job->pgid, SIGCONT) < 0) {
            perror("hush: kill (SIGCONT)");
        }
        job->state = JOB_RUNNING;
    }

    // Wait for the job to report
    wait_for_job(job);

    // Put the shell back in the foreground
    tcsetpgrp(shell_terminal, shell_pgid);

    // Restore the shell's terminal modes
    tcgetattr(shell_terminal, &job->tmodes);
    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
}

// Put a job in the background
void put_job_in_background(Job *job, int cont) {
    // Send the job a continue signal if necessary
    if (cont) {
        if (kill(-job->pgid, SIGCONT) < 0) {
            perror("hush: kill (SIGCONT)");
        }
        job->state = JOB_RUNNING;
    }

    // Report job status
    format_job_info(job, cont ? "Continued" : "Running");
}

// Format job status information
void format_job_info(Job *job, const char *status) {
    fprintf(stderr, "[%d] %s\t\t%s\n", job->id, status, job->command);
}

// Free a job structure and its processes
void free_job(Job *job) {
    if (!job) return;

    // Free all processes
    Process *p = job->first_process;
    while (p) {
        Process *next = p->next;
        free(p);
        p = next;
    }

    // Free command string
    free(job->command);

    // Free job structure
    free(job);
}

// Remove a completed job from the jobs list
void remove_job(int job_id) {
    if (job_id <= 0 || job_id > MAX_JOBS || !jobs[job_id - 1]) {
        return;
    }

    free_job(jobs[job_id - 1]);
    jobs[job_id - 1] = NULL;
}

// Clean up jobs table by removing completed jobs
void cleanup_jobs(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i] && job_is_completed(jobs[i]) && jobs[i]->notified) {
            free_job(jobs[i]);
            jobs[i] = NULL;
        }
    }
}

// Continue a job in foreground/background
void continue_job(Job *job, int foreground) {
    job->foreground = foreground;

    if (foreground) {
        put_job_in_foreground(job, 1);
    } else {
        put_job_in_background(job, 1);
    }
}

// Launch a job (for both foreground and background processes)
int launch_job(Job *job, char **args, int foreground) {
    Process *proc;
    pid_t pid;
    int status = 0;

    // Set default foreground/background state
    job->foreground = foreground;

    // Fork the child process
    pid = fork();

    if (pid == 0) {
        // Child process

        // Put this process in a new process group
        if (shell_is_interactive) {
            pid = getpid();
            if (setpgid(pid, job->pgid) < 0) {
                perror("hush: setpgid");
                exit(EXIT_FAILURE);
            }

            // If foreground job, grab control of terminal
            if (foreground) {
                if (tcsetpgrp(shell_terminal, job->pgid) < 0) {
                    perror("hush: tcsetpgrp");
                }
            }

            // Reset signal handlers
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
            signal(SIGCHLD, SIG_DFL);
        }

        // Execute the command
        if (execvp(args[0], args) < 0) {
            perror("hush: execvp");
            exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS); // Should not reach here
    }
    else if (pid < 0) {
        // Error forking
        perror("hush: fork");
        return -1;
    }
    else {
        // Parent process

        // Add the process to the job
        add_process_to_job(job, pid);

        // Set the process group if it's not set yet
        if (job->pgid == 0) {
            job->pgid = pid;
        }

        // Set process group for the child
        if (shell_is_interactive) {
            if (setpgid(pid, job->pgid) < 0) {
                if (errno != EACCES) {
                    perror("hush: setpgid");
                }
            }
        }

        // If foreground job, wait for it
        if (foreground) {
            put_job_in_foreground(job, 0);

            // Check job status
            if (job_is_completed(job)) {
                // Extract exit status from first process
                Process *p = job->first_process;
                if (p && p->completed && WIFEXITED(p->status)) {
                    status = WEXITSTATUS(p->status);
                }
                remove_job(job->id);
            }
            else if (job_is_stopped(job)) {
                job->state = JOB_STOPPED;
                format_job_info(job, "Stopped");
            }
        }
        else {
            // Background job
            put_job_in_background(job, 0);
        }

        return status;
    }
}

// Implementation of jobs built-in command
int hush_jobs(char **args) {
    int show_pid = 0;

    // Check for -p flag to show PIDs
    if (args[1] && strcmp(args[1], "-p") == 0) {
        show_pid = 1;
    }

    // Update status of all jobs
    update_all_jobs_status();

    // Print job information
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i]) {
            if (show_pid) {
                printf("[%d] %d %s\n",
                       jobs[i]->id,
                       (int)jobs[i]->pgid,
                       jobs[i]->command);
            } else {
                const char *status_str = "Running";
                if (jobs[i]->state == JOB_STOPPED) {
                    status_str = "Stopped";
                } else if (jobs[i]->state == JOB_DONE) {
                    status_str = "Done";
                } else if (jobs[i]->state == JOB_TERMINATED) {
                    status_str = "Terminated";
                }
                printf("[%d] %c %s\t%s\n",
                       jobs[i]->id,
                       (i == 0) ? '+' : ((i == 1) ? '-' : ' '),
                       status_str,
                       jobs[i]->command);
            }
        }
    }

    // Clean up completed jobs
    cleanup_jobs();

    return 1;
}

// Helper function to parse job specifier from arguments
Job *parse_job_spec(char *arg) {
    if (!arg) return NULL;

    // Check if job specifier starts with % (e.g., %1, %2)
    if (arg[0] == '%') {
        // Get job number
        int job_num = atoi(&arg[1]);
        if (job_num <= 0) {
            fprintf(stderr, "hush: invalid job specifier: %s\n", arg);
            return NULL;
        }
        return find_job_by_id(job_num);
    }

    // Default to most recent job if no arg provided
    if (arg[0] == '\0') {
        // Find most recently used job (the one marked with +)
        for (int i = 0; i < MAX_JOBS; i++) {
            if (jobs[i]) {
                return jobs[i];
            }
        }
    }

    fprintf(stderr, "hush: no suitable job found\n");
    return NULL;
}

// Implementation of fg built-in command
int hush_fg(char **args) {
    Job *job = NULL;

    if (args[1]) {
        job = parse_job_spec(args[1]);
    } else {
        // Default to most recently used job
        for (int i = 0; i < MAX_JOBS; i++) {
            if (jobs[i]) {
                job = jobs[i];
                break;
            }
        }
    }

    if (!job) {
        fprintf(stderr, "hush: fg: no current job\n");
        return 1;
    }

    // Continue the job in the foreground
    continue_job(job, 1);

    return 1;
}

// Implementation of bg built-in command
int hush_bg(char **args) {
    Job *job = NULL;

    if (args[1]) {
        job = parse_job_spec(args[1]);
    } else {
        // Default to most recently stopped job
        for (int i = 0; i < MAX_JOBS; i++) {
            if (jobs[i] && jobs[i]->state == JOB_STOPPED) {
                job = jobs[i];
                break;
            }
        }
    }

    if (!job) {
        fprintf(stderr, "hush: bg: no current job\n");
        return 1;
    }

    // Continue the job in the background
    continue_job(job, 0);

    return 1;
}

// Implementation of wait built-in command
int hush_wait(char **args) {
    if (args[1]) {
        // Wait for specific job
        Job *job = parse_job_spec(args[1]);
        if (job) {
            while (!job_is_completed(job)) {
                sleep(1);
                update_all_jobs_status();
            }
        } else {
            return 1;
        }
    } else {
        // Wait for all jobs
        int active_jobs;
        do {
            active_jobs = 0;
            for (int i = 0; i < MAX_JOBS; i++) {
                if (jobs[i] && !job_is_completed(jobs[i])) {
                    active_jobs = 1;
                    break;
                }
            }

            if (active_jobs) {
                sleep(1);
                update_all_jobs_status();
            }
        } while (active_jobs);
    }

    // Clean up completed jobs
    cleanup_jobs();

    return 1;
}

// Implementation of disown built-in command
int hush_disown(char **args) {
    if (!args[1]) {
        fprintf(stderr, "hush: disown: job specification required\n");
        return 1;
    }

    Job *job = parse_job_spec(args[1]);
    if (!job) {
        return 1;
    }

    // Mark the job's slot as NULL without killing the process
    int job_id = job->id;
    free(job->command);
    job->command = NULL;

    Process *p = job->first_process;
    while (p) {
        Process *next = p->next;
        free(p);
        p = next;
    }

    free(job);
    jobs[job_id - 1] = NULL;

    return 1;
}
