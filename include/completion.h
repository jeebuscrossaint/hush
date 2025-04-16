#ifndef COMPLETION_H
#define COMPLETION_H

// Get a list of possible completions for the given input
// Returns an array of strings that must be freed by the caller
// The count of completions is returned in the count parameter
char **get_completions(const char *input, int *count);

// Get the common prefix among all possible completions
// Returns a newly allocated string that must be freed by the caller
char *get_common_completion(const char *input);

// Display a list of possible completions
void display_completions(const char *input);

#endif // COMPLETION_H
