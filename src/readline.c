#include "readline.h"
#include "completion.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

// Readline completion function
static char **hush_completion(const char *text, int start, int end) {
    // Check if the input is empty
    if (!text || text[0] == '\0') {
        // For empty input, don't offer any completions
        return NULL;
    }

    // First, check for path completion (for directories like "inc" → "include/")
    // We need to check if there are directories matching what's typed
    DIR *dir = opendir(".");
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            // Check if it's a directory and matches the text prefix
            if (strncmp(entry->d_name, text, strlen(text)) == 0) {
                struct stat st;
                if (stat(entry->d_name, &st) == 0 && S_ISDIR(st.st_mode)) {
                    // Found a matching directory, let readline handle it
                    closedir(dir);
                    rl_attempted_completion_over = 0;
                    return NULL;
                }
            }
        }
        closedir(dir);
    }

    // Check if we're completing a command or an argument
    int in_command_position = 1;
    for (int i = 0; i < start; i++) {
        if (!isspace((unsigned char)rl_line_buffer[i])) {
            in_command_position = 0;
            break;
        }
    }

    // Check if this looks like a path (contains /, starts with . or ~)
    if (strchr(text, '/') != NULL || text[0] == '.' || text[0] == '~') {
        // Let readline handle path completion
        rl_attempted_completion_over = 0;
        return NULL;
    }

    // If we're not in the command position (typing an argument), use file completion
    if (!in_command_position) {
        rl_attempted_completion_over = 0;
        return NULL;
    }

    // Continue with command completion logic
    rl_attempted_completion_over = 1;

    // Get possible completions
    int count = 0;
    char **completions = get_completions(text, &count);

    // Check if we got any completions
    if (!completions || count == 0) {
        if (completions) free(completions);
        return NULL;
    }

    // Allocate space for matches
    char **matches = malloc((count + 2) * sizeof(char *));
    if (!matches) {
        for (int i = 0; i < count; i++) {
            if (completions[i]) free(completions[i]);
        }
        free(completions);
        return NULL;
    }

    // Build matches array
    matches[0] = strdup(text);
    for (int i = 0; i < count; i++) {
        matches[i + 1] = completions[i];
    }
    matches[count + 1] = NULL;

    free(completions);
    return matches;
}

// Initialize readline with our custom settings
void hush_readline_init(void) {
    // Set readline completion function
    rl_attempted_completion_function = hush_completion;

    // Configure tab to complete on first press and display on second press
    rl_completion_query_items = 100;  // Only ask to display if there are many completions
    rl_bind_key('\t', rl_complete);   // Explicit binding of tab to rl_complete
    rl_completion_display_matches_hook = NULL; // Use default display function
    rl_variable_bind("show-all-if-ambiguous", "off"); // Only show on second tab

    // Don't append a space after completion
    rl_completion_append_character = '\0';

    // Don't use default filename completion
    rl_completion_entry_function = NULL;

    // Set the prompt
    rl_readline_name = "hush";

    // Read history file
    read_history(NULL);
}

// Read a line using readline
char *hush_read_line(void) {
    // Set the prompt
    char *line = readline("$ ");

    // If line is not empty, add it to history
    if (line && *line) {
        add_history(line);
    }

    return line;
}

// Clean up readline
void hush_readline_cleanup(void) {
    // Write history to file
    write_history(NULL); // You might want to provide a specific filename
}
