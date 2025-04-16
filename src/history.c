#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>

// Initialize the history variables
char *history_list[HISTORY_MAX];
int history_count = 0;

// Add a command to the history
void add_to_history(char *line) {
    // Skip empty lines
    if (line == NULL || line[0] == '\0') {
        return;
    }

    // Skip if the line is the same as the last command
    if (history_count > 0 && strcmp(line, history_list[history_count-1]) == 0) {
        return;
    }

    // If we're at max capacity, remove the oldest entry
    if (history_count >= HISTORY_MAX) {
        free(history_list[0]);
        // Shift all elements left
        for (int i = 1; i < HISTORY_MAX; i++) {
            history_list[i-1] = history_list[i];
        }
        history_count--;
    }

    // Add the new command
    history_list[history_count] = strdup(line);
    if (history_list[history_count] == NULL) {
        perror("hush: strdup error in add_to_history");
        return;
    }

    history_count++;
}

// Get the path to the history file
char *get_history_file_path() {
    // Get user's home directory
    const char *home_dir;
    if ((home_dir = getenv("HOME")) == NULL) {
        home_dir = getpwuid(getuid())->pw_dir;
    }

    // Create the full path
    char *path = malloc(strlen(home_dir) + 20); // Extra space for "/.hush_history\0"
    if (path == NULL) {
        perror("hush: malloc error in get_history_file_path");
        return NULL;
    }

    sprintf(path, "%s/.hush_history", home_dir);
    return path;
}

// Save history to file
void save_history() {
    char *path = get_history_file_path();
    if (path == NULL) {
        return;
    }

    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        perror("hush: error opening history file for writing");
        free(path);
        return;
    }

    for (int i = 0; i < history_count; i++) {
        fprintf(fp, "%s\n", history_list[i]);
    }

    fclose(fp);
    free(path);
}

// Load history from file
void load_history() {
    char *path = get_history_file_path();
    if (path == NULL) {
        return;
    }

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        // It's ok if the file doesn't exist yet
        if (errno != ENOENT) {
            perror("hush: error opening history file for reading");
        }
        free(path);
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), fp) != NULL && history_count < HISTORY_MAX) {
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        // Add to history
        history_list[history_count] = strdup(line);
        if (history_list[history_count] == NULL) {
            perror("hush: strdup error in load_history");
            break;
        }

        history_count++;
    }

    fclose(fp);
    free(path);
}

// The history builtin command
int hush_history(char **args) {
    // Simple implementation - just display the history
    for (int i = 0; i < history_count; i++) {
        printf("%5d  %s\n", i + 1, history_list[i]);
    }

    return 1;
}

// Get a specific history entry by number
char *get_history_entry(int index) {
    if (index < 1 || index > history_count) {
        return NULL;
    }

    return strdup(history_list[index - 1]);
}

// Expand history references (e.g., !!, !42, !-3)
char *expand_history(char *line) {
    // Skip empty lines
    if (line == NULL || line[0] == '\0') {
        return strdup("");
    }

    // Check for history expansion
    if (line[0] == '!') {
        // !!, repeat the last command
        if (line[1] == '!' && history_count > 0) {
            return strdup(history_list[history_count - 1]);
        }

        // !n, repeat the nth command
        if (isdigit(line[1])) {
            int index = atoi(line + 1);
            char *entry = get_history_entry(index);
            if (entry != NULL) {
                return entry;
            }
        }

        // !-n, repeat the nth previous command
        if (line[1] == '-' && isdigit(line[2])) {
            int offset = atoi(line + 2);
            if (offset > 0 && offset <= history_count) {
                return strdup(history_list[history_count - offset]);
            }
        }
    }

    // No expansion, return a copy of the original
    return strdup(line);
}
