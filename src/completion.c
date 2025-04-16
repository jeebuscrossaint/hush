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

// Generate file completions
static char **generate_completions(const char *input, int *count) {
    // Initialize return values
    *count = 0;

    // Check for empty input
    if (!input || input[0] == '\0') {
        char **result = malloc(sizeof(char *));
        if (!result) {
            perror("hush: allocation error in completion");
            return NULL;
        }
        result[0] = NULL;
        return result;
    }

    // Determine directory to search and prefix to match
    char *dir_path = NULL;
    char *file_prefix = NULL;
    parse_path(input, &dir_path, &file_prefix);

    // Open the directory
    DIR *dir = opendir(dir_path);
    if (!dir) {
        free(dir_path);
        free(file_prefix);
        char **result = malloc(sizeof(char *));
        if (!result) {
            perror("hush: allocation error in completion");
            return NULL;
        }
        result[0] = NULL;
        return result;
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
    char **result = malloc((completions.count + 1) * sizeof(char *));
    if (!result) {
        perror("hush: allocation error in completion");
        completion_free(&completions);
        free(dir_path);
        free(file_prefix);
        return NULL;
    }

    // Move items to result
    for (int i = 0; i < completions.count; i++) {
        result[i] = completions.items[i];
    }
    result[completions.count] = NULL;

    // Set return count
    *count = completions.count;

    // Clean up
    free(completions.items);  // Don't free the strings as they're now owned by result
    free(dir_path);
    free(file_prefix);

    return result;
}

// Get the completion to show based on current input
char **get_completions(const char *input, int *count) {
    return generate_completions(input, count);
}

// Get the common prefix of completions
char *get_common_completion(const char *input) {
    int count;
    char **completions = generate_completions(input, &count);

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

// Display a list of possible completions
void display_completions(const char *input) {
    int count;
    char **completions = get_completions(input, &count);

    if (count == 0) {
        putchar('\a');  // Terminal bell for no completions
        free(completions);
        return;
    }

    if (count == 1) {
        // Only one completion, don't show a list
        free(completions[0]);
        free(completions);
        return;
    }

    // Calculate display width based on completions
    int longest = 0;
    for (int i = 0; i < count; i++) {
        int len = strlen(completions[i]);
        if (len > longest) {
            longest = len;
        }
    }
    longest += 2;  // Add space between columns

    // Get terminal width
    int term_width = 80;  // Default
    char *columns_env = getenv("COLUMNS");
    if (columns_env) {
        term_width = atoi(columns_env);
        if (term_width <= 0) term_width = 80;
    }

    // Calculate columns and rows
    int columns = term_width / longest;
    if (columns == 0) columns = 1;
    int rows = (count + columns - 1) / columns;

    // Print a newline to start listing below the prompt
    printf("\n");

    // Display in columns
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < columns; col++) {
            int index = col * rows + row;
            if (index < count) {
                printf("%-*s", longest, completions[index]);
            }
        }
        printf("\n");
    }

    // Clean up
    for (int i = 0; i < count; i++) {
        free(completions[i]);
    }
    free(completions);
}
