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

    // Get possible completions from our existing code
    int count;
    return get_completions(text, &count);
}

// Initialize readline with our custom settings
void hush_readline_init(void) {
    // Set readline completion function
    rl_attempted_completion_function = hush_completion;

    // Display matches in a single column
    rl_completion_display_matches_hook = NULL;

    // Don't use default filename completion
    rl_completion_entry_function = NULL;

    // Set the prompt
    rl_readline_name = "hush";

    // Read history file
    read_history(NULL); // You might want to provide a specific filename
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
