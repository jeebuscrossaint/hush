#include "glob.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fnmatch.h>
#include <ctype.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>

// Vector for matches
typedef struct {
    char **items;
    int count;
    int capacity;
} match_vector;

static void vector_init(match_vector *v) {
    v->items = malloc(10 * sizeof(char *));
    v->count = 0;
    v->capacity = 10;
}

static void vector_add(match_vector *v, const char *item) {
    if (v->count >= v->capacity) {
        v->capacity *= 2;
        v->items = realloc(v->items, v->capacity * sizeof(char *));
    }
    v->items[v->count++] = strdup(item);
}

static void vector_free(match_vector *v) {
    free(v->items);
    v->items = NULL;
    v->count = v->capacity = 0;
}

// Free a list of strings
static void free_string_array(char **array, int count) {
    for (int i = 0; i < count; i++) {
        free(array[i]);
    }
    free(array);
}

// Check if a pattern contains globbing characters
int has_wildcards(const char *pattern) {
    return (strchr(pattern, '*') != NULL ||
            strchr(pattern, '?') != NULL ||
            strchr(pattern, '[') != NULL ||
            strchr(pattern, '{') != NULL);
}

// Check if a pattern contains extended globbing characters
static int has_extended_glob(const char *pattern) {
    // Check for brace expansion, recursive glob, extended patterns
    return (strchr(pattern, '{') != NULL ||
            strstr(pattern, "**") != NULL ||
            strstr(pattern, "!(") != NULL ||
            strstr(pattern, "?(") != NULL ||
            strstr(pattern, "*(") != NULL ||
            strstr(pattern, "+(") != NULL ||
            strstr(pattern, "@(") != NULL);
}

// Check if a pattern contains extended pattern operators
static int is_extended_pattern(const char *pattern) {
    if (strlen(pattern) < 3) return 0;

    // Check for !(pattern), ?(pattern), etc.
    if ((pattern[0] == '!' || pattern[0] == '?' ||
         pattern[0] == '*' || pattern[0] == '+' ||
         pattern[0] == '@') && pattern[1] == '(') {
        return 1;
    }
    return 0;
}

// Process brace expansion like {a,b,c}
static char **expand_braces(const char *pattern, int *count) {
    // Find the first opening brace
    const char *open = strchr(pattern, '{');
    if (!open) {
        // No braces to expand
        *count = 1;
        char **result = malloc(sizeof(char *));
        result[0] = strdup(pattern);
        return result;
    }

    // Find the matching closing brace
    const char *close = NULL;
    int nested = 0;
    for (const char *p = open + 1; *p; p++) {
        if (*p == '{') {
            nested++;
        } else if (*p == '}') {
            if (nested == 0) {
                close = p;
                break;
            }
            nested--;
        }
    }

    if (!close) {
        // No matching closing brace
        *count = 1;
        char **result = malloc(sizeof(char *));
        result[0] = strdup(pattern);
        return result;
    }

    // Extract the prefix and suffix
    char prefix[PATH_MAX];
    strncpy(prefix, pattern, open - pattern);
    prefix[open - pattern] = '\0';

    char suffix[PATH_MAX];
    strcpy(suffix, close + 1);

    // Extract and expand the comma-separated list
    char brace_content[PATH_MAX];
    strncpy(brace_content, open + 1, close - open - 1);
    brace_content[close - open - 1] = '\0';

    // Parse the comma-separated options
    match_vector options;
    vector_init(&options);

    char *token = strtok(brace_content, ",");
    while (token) {
        vector_add(&options, token);
        token = strtok(NULL, ",");
    }

    // Combine prefix, each option, and suffix
    match_vector results;
    vector_init(&results);

    for (int i = 0; i < options.count; i++) {
        char new_pattern[PATH_MAX];
        snprintf(new_pattern, sizeof(new_pattern), "%s%s%s",
                 prefix, options.items[i], suffix);

        // Recursively expand any remaining braces
        int sub_count;
        char **sub_patterns = expand_braces(new_pattern, &sub_count);

        for (int j = 0; j < sub_count; j++) {
            vector_add(&results, sub_patterns[j]);
        }

        free_string_array(sub_patterns, sub_count);
    }

    *count = results.count;
    char **result = results.items;

    vector_free(&options);
    return result;
}

// Get all matching files in a directory for a pattern
static void match_files_in_dir(const char *dir_path, const char *pattern, match_vector *matches) {
    DIR *dir = opendir(dir_path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and .. (unless explicitly requested)
        if (strcmp(entry->d_name, ".") == 0 ||
            (strcmp(entry->d_name, "..") == 0 && strcmp(pattern, "..") != 0)) {
            continue;
        }

        // Check if the file matches the pattern
        if (fnmatch(pattern, entry->d_name, FNM_PATHNAME) == 0) {
            char full_path[PATH_MAX];
            if (strcmp(dir_path, ".") == 0) {
                strcpy(full_path, entry->d_name);
            } else {
                snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
            }
            vector_add(matches, full_path);
        }
    }

    closedir(dir);
}

// Recursive directory traversal for ** patterns
static void traverse_directories(const char *base_path, const char *pattern, match_vector *matches) {
    // First check the current directory
    match_files_in_dir(base_path, pattern, matches);

    // Then recurse into subdirectories
    DIR *dir = opendir(base_path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Check if it's a directory
        char path[PATH_MAX];
        if (strcmp(base_path, ".") == 0) {
            strcpy(path, entry->d_name);
        } else {
            snprintf(path, sizeof(path), "%s/%s", base_path, entry->d_name);
        }

        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            traverse_directories(path, pattern, matches);
        }
    }

    closedir(dir);
}

// Process a single glob pattern
static char **process_glob_pattern(const char *pattern, int *count) {
    match_vector matches;
    vector_init(&matches);

    // Handle special case for ** recursive glob
    if (strstr(pattern, "**") != NULL) {
        char *dir_part = strdup(pattern);
        char *star_pos = strstr(dir_part, "**");

        // Extract the directory part before **
        *star_pos = '\0';
        char *dir_path = (*dir_part == '\0') ? "." : dir_part;

        // Extract the pattern after **
        char file_pattern[PATH_MAX] = "*";
        if (*(star_pos + 2) != '\0') {
            strcpy(file_pattern, star_pos + 3); // +3 to skip "**/"
        }

        // Recursively traverse directories
        traverse_directories(dir_path, file_pattern, &matches);
        free(dir_part);
    } else {
        // Extract directory and file parts
        char *pattern_copy = strdup(pattern);
        char *dir_path = ".";
        char *file_pattern = pattern_copy;

        // Find the last slash to split dir and file parts
        char *last_slash = strrchr(pattern_copy, '/');
        if (last_slash) {
            *last_slash = '\0';
            dir_path = (*pattern_copy == '\0') ? "." : pattern_copy;
            file_pattern = last_slash + 1;
        }

        // Match files in the directory
        match_files_in_dir(dir_path, file_pattern, &matches);
        free(pattern_copy);
    }

    // If no matches found, return the original pattern
    if (matches.count == 0) {
        vector_add(&matches, pattern);
    }

    *count = matches.count;
    return matches.items;
}

// Main function to expand a wildcard pattern
char **expand_wildcard(char *pattern, int *count) {
    // First handle brace expansion
    int brace_count;
    char **brace_expanded = expand_braces(pattern, &brace_count);

    // Then process each brace-expanded pattern for wildcards
    match_vector all_matches;
    vector_init(&all_matches);

    for (int i = 0; i < brace_count; i++) {
        if (has_wildcards(brace_expanded[i])) {
            int glob_count;
            char **glob_matches = process_glob_pattern(brace_expanded[i], &glob_count);

            for (int j = 0; j < glob_count; j++) {
                vector_add(&all_matches, glob_matches[j]);
            }

            free_string_array(glob_matches, glob_count);
        } else {
            vector_add(&all_matches, brace_expanded[i]);
        }
    }

    free_string_array(brace_expanded, brace_count);

    *count = all_matches.count;
    return all_matches.items;
}

// Expand all arguments that contain wildcards
char **expand_wildcards(char **args) {
    if (!args || !args[0]) {
        return args;
    }

    // Count the original arguments
    int arg_count = 0;
    while (args[arg_count] != NULL) {
        arg_count++;
    }

    // Initial capacity for expanded args
    int capacity = arg_count * 2;
    char **expanded_args = malloc(capacity * sizeof(char *));
    if (!expanded_args) {
        perror("hush: allocation error");
        exit(EXIT_FAILURE);
    }

    // Process each argument
    int expanded_count = 0;
    for (int i = 0; i < arg_count; i++) {
        // Skip empty arguments
        if (!args[i] || args[i][0] == '\0') {
            continue;
        }

        // Check if this argument has wildcards
        if (has_wildcards(args[i])) {
            int match_count;
            char **matches = expand_wildcard(args[i], &match_count);

            // Ensure we have enough capacity
            if (expanded_count + match_count >= capacity) {
                capacity = (expanded_count + match_count) * 2;
                expanded_args = realloc(expanded_args, capacity * sizeof(char *));
                if (!expanded_args) {
                    perror("hush: allocation error");
                    exit(EXIT_FAILURE);
                }
            }

            // Add the matches to our result
            for (int j = 0; j < match_count; j++) {
                expanded_args[expanded_count++] = matches[j];
            }

            // Free the matches array (but not the strings)
            free(matches);
        } else {
            // No wildcards, just copy the argument
            if (expanded_count >= capacity) {
                capacity *= 2;
                expanded_args = realloc(expanded_args, capacity * sizeof(char *));
                if (!expanded_args) {
                    perror("hush: allocation error");
                    exit(EXIT_FAILURE);
                }
            }

            expanded_args[expanded_count++] = strdup(args[i]);
        }
    }

    // Null-terminate the result
    expanded_args[expanded_count] = NULL;

    return expanded_args;
}
