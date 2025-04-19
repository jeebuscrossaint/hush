#include "completion.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <pwd.h>
#include <ctype.h>
#include <errno.h>

#define MANY_COMPLETIONS_THRESHOLD 100

// Define data structure for completions
typedef struct {
    char **items;
    int count;
    int capacity;
} completion_list;

// Initialize completion list
static void completion_init(completion_list *list) {
    list->capacity = 10;
    list->count = 0;
    list->items = malloc(list->capacity * sizeof(char *));
    if (!list->items) {
        perror("hush: allocation error in completion");
        exit(EXIT_FAILURE);
    }
}

// Add an item to the completion list
static void completion_add(completion_list *list, const char *item) {
    // Skip . and .. if at the beginning of a completion list
    // (don't hide them if the user explicitly types . or ..)
    if (list->count == 0 &&
        (strcmp(item, ".") == 0 || strcmp(item, "..") == 0)) {
        return;
    }

    // Check if item already exists in list (avoid duplicates)
    for (int i = 0; i < list->count; i++) {
        if (strcmp(list->items[i], item) == 0) {
            return;
        }
    }

    // Resize list if needed
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, list->capacity * sizeof(char *));
        if (!list->items) {
            perror("hush: allocation error in completion");
            exit(EXIT_FAILURE);
        }
    }

    // Add the item
    list->items[list->count] = strdup(item);
    if (!list->items[list->count]) {
        perror("hush: allocation error in completion");
        exit(EXIT_FAILURE);
    }
    list->count++;
}

// Free completion list
static void completion_free(completion_list *list) {
    for (int i = 0; i < list->count; i++) {
        free(list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

// Compare function for qsort to sort completions alphabetically
static int completion_compare(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

// Find the common prefix of a set of strings
static char *find_common_prefix(char **items, int count) {
    if (count <= 0) return strdup("");
    if (count == 1) return strdup(items[0]);

    // Start with the first item as the initial prefix
    char *prefix = strdup(items[0]);
    int prefix_len = strlen(prefix);

    // Compare each item against the current prefix
    for (int i = 1; i < count; i++) {
        int j;
        for (j = 0; j < prefix_len && items[i][j] == prefix[j]; j++) {
            // Continue while characters match
        }
        // Update prefix length to the matching portion
        prefix_len = j;
        prefix[prefix_len] = '\0';

        // If prefix is empty, no need to continue
        if (prefix_len == 0) break;
    }

    return prefix;
}

// Expand tilde in path
static char *expand_tilde(const char *path) {
    if (!path || path[0] != '~') {
        return path ? strdup(path) : strdup("");
    }

    const char *home = NULL;

    // ~user case
    if (path[1] != '/' && path[1] != '\0') {
        // Extract username
        const char *slash = strchr(path, '/');
        int username_len = slash ? (slash - path - 1) : strlen(path) - 1;
        char username[256];
        strncpy(username, path + 1, username_len);
        username[username_len] = '\0';

        // Get user's home directory
        struct passwd *pw = getpwnam(username);
        if (pw) {
            home = pw->pw_dir;
        } else {
            // User not found, return original
            return strdup(path);
        }
    } else {
        // Simple ~ case, use current user's home
        home = getenv("HOME");
        if (!home) {
            struct passwd *pw = getpwuid(getuid());
            if (pw) {
                home = pw->pw_dir;
            } else {
                // Can't determine home, return original
                return strdup(path);
            }
        }
    }

    // Calculate final path size
    int path_offset = (path[1] == '/' ? 1 : (strchr(path, '/') ? strchr(path, '/') - path : strlen(path)));
    int result_size = strlen(home) + strlen(path) - path_offset + 1;

    // Create and fill the result
    char *result = malloc(result_size);
    if (!result) {
        perror("hush: allocation error in tilde expansion");
        return strdup(path);
    }

    strcpy(result, home);
    strcat(result, path + path_offset);

    return result;
}

// Extract directory and file prefix from a path
static void parse_path(const char *input, char **dir_path, char **file_prefix) {
    // Expand tilde if present
    char *expanded = expand_tilde(input);

    // Find the last slash to split directory and file prefix
    char *last_slash = strrchr(expanded, '/');

    if (last_slash) {
        // There's a directory component
        *last_slash = '\0';  // Temporarily terminate the string at the slash
        *dir_path = (*expanded == '\0') ? strdup("/") : strdup(expanded);
        *file_prefix = strdup(last_slash + 1);
        *last_slash = '/';  // Restore the slash
    } else {
        // No directory component, use current directory
        *dir_path = strdup(".");
        *file_prefix = strdup(expanded);
    }

    free(expanded);
}

// Get executables from PATH for command completion
static void add_executables_from_path(completion_list *completions, const char *prefix) {
    char *path = getenv("PATH");
    if (!path) return;

    // Make a copy of PATH since strtok modifies its input
    char *path_copy = strdup(path);
    if (!path_copy) {
        perror("hush: strdup error");
        return;
    }

    int prefix_len = strlen(prefix);
    char *dir = strtok(path_copy, ":");

    while (dir) {
        DIR *d = opendir(dir);
        if (d) {
            struct dirent *entry;
            while ((entry = readdir(d)) != NULL) {
                // Skip hidden files and directories
                if (entry->d_name[0] == '.') continue;

                // Check if name starts with prefix
                if (strncmp(entry->d_name, prefix, prefix_len) == 0) {
                    // Build full path to check if it's executable
                    char full_path[PATH_MAX];
                    snprintf(full_path, PATH_MAX, "%s/%s", dir, entry->d_name);

                    struct stat st;
                    if (stat(full_path, &st) == 0) {
                        // Check if it's a regular file and executable
                        if (S_ISREG(st.st_mode) &&
                            (st.st_mode & S_IXUSR || st.st_mode & S_IXGRP || st.st_mode & S_IXOTH)) {
                            completion_add(completions, entry->d_name);
                        }
                    }
                }
            }
            closedir(d);
        }
        dir = strtok(NULL, ":");
    }

    free(path_copy);
}

// Get the completion to show based on current input
char **get_completions(const char *input, int *count) {
    char *dir_path = NULL;
    char *file_prefix = NULL;

    // Setup default return values
    *count = 0;

    // Check if empty or first word
    int is_first_word = 1;
    const char *check = input;
    while (*check && isspace(*check)) check++; // Skip leading spaces

    while (*check && !isspace(*check)) check++; // Find end of word
    while (*check && isspace(*check)) check++; // Skip spaces after word

    // If we found more content, it's not the first word
    if (*check) is_first_word = 0;

    if (!input || input[0] == '\0') {
        // Empty input - list executables in PATH
        completion_list completions;
        completion_init(&completions);

        add_executables_from_path(&completions, "");

        // Sort completions
        qsort(completions.items, completions.count, sizeof(char *), completion_compare);

        // Prepare the result array
        char **result = NULL;
        if (completions.count > 0) {
            result = malloc((completions.count + 1) * sizeof(char *));
            if (!result) {
                perror("hush: allocation error in completion");
                completion_free(&completions);
                *count = 0;
                return NULL;
            }

            // Copy items to result
            for (int i = 0; i < completions.count; i++) {
                result[i] = strdup(completions.items[i]);
            }
            result[completions.count] = NULL;
        } else {
            // No completions
            result = malloc(sizeof(char *));
            result[0] = NULL;
        }

        *count = completions.count;
        completion_free(&completions);
        return result;
    }

    // Parse the input into directory and file prefix
    parse_path(input, &dir_path, &file_prefix);

    // If this is the first word and doesn't start with ./ or / or ~,
    // it's likely a command rather than a file
    if (is_first_word &&
        input[0] != '.' && input[0] != '/' && input[0] != '~') {

        completion_list completions;
        completion_init(&completions);

        add_executables_from_path(&completions, file_prefix);

        // Sort completions
        qsort(completions.items, completions.count, sizeof(char *), completion_compare);

        // Prepare the result array
        char **result = NULL;
        if (completions.count > 0) {
            result = malloc((completions.count + 1) * sizeof(char *));
            if (!result) {
                perror("hush: allocation error in completion");
                completion_free(&completions);
                free(dir_path);
                free(file_prefix);
                *count = 0;
                return NULL;
            }

            // Copy items to result
            for (int i = 0; i < completions.count; i++) {
                result[i] = strdup(completions.items[i]);
            }
            result[completions.count] = NULL;
        } else {
            // No completions
            result = malloc(sizeof(char *));
            result[0] = NULL;
        }

        *count = completions.count;
        completion_free(&completions);
        free(dir_path);
        free(file_prefix);
        return result;
    }

    // Open the directory
    DIR *dir = opendir(dir_path);
    if (!dir) {
        free(dir_path);
        free(file_prefix);
        *count = 0;
        return malloc(sizeof(char *)); // Return empty array
    }

    // Gather matching entries
    completion_list completions;
    completion_init(&completions);

    struct dirent *entry;
    int prefix_len = strlen(file_prefix);

    while ((entry = readdir(dir)) != NULL) {
        // Skip hidden files unless the prefix starts with a dot
        if (entry->d_name[0] == '.' && file_prefix[0] != '.') {
            continue;
        }

        // Check if entry name starts with our prefix
        if (strncmp(entry->d_name, file_prefix, prefix_len) == 0) {
            // Build the full path for stat
            char full_path[PATH_MAX];
            if (strcmp(dir_path, "/") == 0) {
                snprintf(full_path, PATH_MAX, "/%s", entry->d_name);
            } else if (strcmp(dir_path, ".") == 0) {
                snprintf(full_path, PATH_MAX, "%s", entry->d_name);
            } else {
                snprintf(full_path, PATH_MAX, "%s/%s", dir_path, entry->d_name);
            }

            // Check if it's a directory and add a trailing slash if so
            struct stat st;
            if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                char *name_with_slash = malloc(strlen(entry->d_name) + 2);
                if (name_with_slash) {
                    strcpy(name_with_slash, entry->d_name);
                    strcat(name_with_slash, "/");
                    completion_add(&completions, name_with_slash);
                    free(name_with_slash);
                }
            } else {
                completion_add(&completions, entry->d_name);
            }
        }
    }

    closedir(dir);

    // Sort completions alphabetically
    qsort(completions.items, completions.count, sizeof(char *), completion_compare);

    // Prepare the result array
    char **result = NULL;
    if (completions.count > 0) {
        result = malloc((completions.count + 1) * sizeof(char *));
        if (!result) {
            perror("hush: allocation error in completion");
            completion_free(&completions);
            free(dir_path);
            free(file_prefix);
            *count = 0;
            return NULL;
        }

        // Move items to result
        for (int i = 0; i < completions.count; i++) {
            result[i] = strdup(completions.items[i]);
        }
        result[completions.count] = NULL;
    } else {
        // No completions found
        result = malloc(sizeof(char *));
        result[0] = NULL;
    }

    // Set return count
    *count = completions.count;

    // Clean up
    completion_free(&completions);
    free(dir_path);
    free(file_prefix);

    return result;
}

// Get the common prefix of completions
char *get_common_completion(const char *input) {
    int count;
    char **completions = get_completions(input, &count);

    if (count == 0) {
        free(completions);
        return strdup(input);
    }

    char *common = find_common_prefix(completions, count);

    // If there's a directory part in the input, we need to add it back
    char *dir_path = NULL;
    char *file_prefix = NULL;
    parse_path(input, &dir_path, &file_prefix);

    char *result;
    if (strcmp(dir_path, ".") != 0) {
        char *expanded_dir = expand_tilde(dir_path);
        result = malloc(strlen(expanded_dir) + strlen(common) + 2);
        if (result) {
            if (strcmp(expanded_dir, "/") == 0) {
                sprintf(result, "/%s", common);
            } else {
                sprintf(result, "%s/%s", expanded_dir, common);
            }
        }
        free(expanded_dir);
    } else {
        result = strdup(common);
    }

    // Clean up
    free(common);
    for (int i = 0; i < count; i++) {
        free(completions[i]);
    }
    free(completions);
    free(dir_path);
    free(file_prefix);

    return result;
}

// Add this new function to format completions in nice columns - fixed version
static void display_completions_in_columns(char **completions, int count) {
    if (count == 0) return;

    // Get terminal width
    int term_width = 80; // Default width
    char *columns_env = getenv("COLUMNS");
    if (columns_env) {
        term_width = atoi(columns_env);
        if (term_width <= 0) term_width = 80;
    }

    // Find the maximum completion length
    int max_len = 0;
    for (int i = 0; i < count; i++) {
        int len = strlen(completions[i]);
        if (len > max_len) max_len = len;
    }

    // Calculate column width (length + padding)
    int col_width = max_len + 2;

    // Calculate how many columns can fit in the terminal
    int num_cols = term_width / col_width;
    if (num_cols == 0) num_cols = 1; // Ensure at least one column

    // Sort completions alphabetically
    qsort(completions, count, sizeof(char *), completion_compare);

    // Calculate number of rows needed
    int num_rows = (count + num_cols - 1) / num_cols;

    // Prepare a buffer for formatted output
    char *line_buf = malloc(term_width + 1);
    if (!line_buf) {
        perror("hush: allocation error in completion display");
        return;
    }

    // Print completions in columns, filling across rows first
    for (int row = 0; row < num_rows; row++) {
        memset(line_buf, ' ', term_width);
        line_buf[term_width] = '\0';

        int line_pos = 0;
        for (int col = 0; col < num_cols; col++) {
            int index = row * num_cols + col;
            if (index < count) {
                int len = strlen(completions[index]);
                if (line_pos + len <= term_width) {
                    memcpy(line_buf + line_pos, completions[index], len);
                    line_pos += col_width;
                }
            }
        }

        // Trim trailing spaces
        int end = term_width - 1;
        while (end >= 0 && line_buf[end] == ' ') end--;
        line_buf[end + 1] = '\0';

        // Write the line
        printf("%s\n", line_buf);
    }

    free(line_buf);
}

// Now replace the existing display_completions function with this improved version
void display_completions(const char *input) {
    int count;
    char **completions = get_completions(input, &count);

    if (count == 0) {
        ssize_t ret = write(STDOUT_FILENO, "\a", 1);  // Terminal bell for no completions
        (void)ret; // Suppress warning
        free(completions);
        return;
    }

    if (count == 1) {
        // Only one completion, don't show a list
        free(completions[0]);
        free(completions);
        return;
    }

    // If there are many completions, ask before displaying them
    if (count > MANY_COMPLETIONS_THRESHOLD) {
        char prompt[64];
        int prompt_len = snprintf(prompt, sizeof(prompt),
                                "Display all %d possibilities? (y or n) ", count);

        ssize_t ret = write(STDOUT_FILENO, prompt, prompt_len);
        (void)ret; // Suppress warning

        // Read user's response
        char c = 0;
        while (c != 'y' && c != 'Y' && c != 'n' && c != 'N') {
            ret = read(STDIN_FILENO, &c, 1);
            (void)ret; // Suppress warning
        }

        // Write newline
        ret = write(STDOUT_FILENO, "\n", 1);
        (void)ret; // Suppress warning

        if (c == 'n' || c == 'N') {
            // User said no, clean up and return
            for (int i = 0; i < count; i++) {
                free(completions[i]);
            }
            free(completions);
            return;
        }
    }

    // Print a newline before completions
    printf("\n");

    // Display completions in nice columns
    display_completions_in_columns(completions, count);

    // Clean up
    for (int i = 0; i < count; i++) {
        free(completions[i]);
    }
    free(completions);
}
