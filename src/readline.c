#include "readline.h"
#include "completion.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

// Readline completion function
static char **hush_completion(const char *text, int start, int end) {
    // This prevents readline from doing filename completion
    rl_attempted_completion_over = 1;

    // Safety check for NULL or empty text
    if (!text) {
        return NULL;  // Let readline handle it
    }

    // Get possible completions
    int count = 0;
    char **completions = get_completions(text, &count);

    // Check if we got any completions
    if (!completions || count == 0) {
        // No completions found
        if (completions) free(completions);

        // Return NULL to indicate no completions
        return NULL;
    }

    // Allocate space for matches in readline's expected format
    char **matches = malloc((count + 2) * sizeof(char *));
    if (!matches) {
        // Memory allocation failed
        for (int i = 0; i < count; i++) {
            if (completions[i]) free(completions[i]);
        }
        free(completions);
        return NULL;
    }

    // Populate matches array in readline's expected format
    matches[0] = strdup(text);  // First entry is the partial text

    // Copy completions to matches (offset by 1)
    for (int i = 0; i < count; i++) {
        if (completions[i]) {
            matches[i + 1] = completions[i];  // Transfer ownership
        } else {
            matches[i + 1] = strdup("");  // Safety fallback
        }
    }

    matches[count + 1] = NULL;  // NULL terminate the array

    // Only free the array itself, not the strings
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
