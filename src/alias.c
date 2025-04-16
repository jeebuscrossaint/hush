#include "alias.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Store aliases in a simple array (could be upgraded to a hash table later)
#define MAX_ALIASES 100

static Alias aliases[MAX_ALIASES];
static int alias_count = 0;

// Find an alias by name
static Alias *find_alias(const char *name) {
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
            return &aliases[i];
        }
    }
    return NULL;
}

// Add or update an alias
int add_alias(const char *name, const char *value) {  // REMOVED static
    if (alias_count >= MAX_ALIASES) {
        fprintf(stderr, "hush: maximum number of aliases reached\n");
        return 0;
    }

    // Check if the alias already exists
    Alias *existing = find_alias(name);
    if (existing) {
        // Update existing alias
        free(existing->value);
        existing->value = strdup(value);
        return 1;
    }

    // Add new alias
    aliases[alias_count].name = strdup(name);
    aliases[alias_count].value = strdup(value);
    alias_count++;
    return 1;
}

// Remove an alias
int remove_alias(const char *name) {  // REMOVED static
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
            // Free memory
            free(aliases[i].name);
            free(aliases[i].value);

            // Move all subsequent aliases one position back
            for (int j = i; j < alias_count - 1; j++) {
                aliases[j] = aliases[j + 1];
            }

            alias_count--;
            return 1;
        }
    }
    return 0;
}

// Show all aliases
static void show_aliases() {
    for (int i = 0; i < alias_count; i++) {
        printf("alias %s='%s'\n", aliases[i].name, aliases[i].value);
    }
}

// The alias builtin command
int hush_alias(char **args) {
    // If no arguments, show all aliases
    if (args[1] == NULL) {
        show_aliases();
        return 1;
    }

    // Process each argument
    for (int i = 1; args[i] != NULL; i++) {
        char *equals = strchr(args[i], '=');

        if (equals == NULL) {
            // No equals sign - show this specific alias
            Alias *alias = find_alias(args[i]);
            if (alias) {
                printf("alias %s='%s'\n", alias->name, alias->value);
            } else {
                fprintf(stderr, "hush: alias: %s: not found\n", args[i]);
            }
        } else {
            // Parse "name=value" format
            *equals = '\0'; // Temporarily split the string
            char *name = args[i];
            char *value = equals + 1;

            // Remove surrounding quotes from value if present
            int len = strlen(value);
            if (len >= 2) {
                if ((value[0] == '\'' && value[len-1] == '\'') ||
                    (value[0] == '"' && value[len-1] == '"')) {
                    value[len-1] = '\0';
                    value++;
                }
            }

            add_alias(name, value);
            *equals = '='; // Restore the string
        }
    }

    return 1;
}

// The unalias builtin command
int hush_unalias(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "hush: unalias: usage: unalias name [name ...]\n");
        return 1;
    }

    for (int i = 1; args[i] != NULL; i++) {
        if (strcmp(args[i], "-a") == 0) {
            // Remove all aliases
            for (int j = 0; j < alias_count; j++) {
                free(aliases[j].name);
                free(aliases[j].value);
            }
            alias_count = 0;
        } else if (!remove_alias(args[i])) {
            fprintf(stderr, "hush: unalias: %s: not found\n", args[i]);
        }
    }

    return 1;
}

// Expand aliases in the arguments array
char **expand_aliases(char **args) {
    if (args == NULL || args[0] == NULL) {
        return args;
    }

    // Check if the command matches an alias
    Alias *alias = find_alias(args[0]);
    if (!alias) {
        return args;  // No alias found, return original args
    }

    // Parse the alias value into tokens
    char *alias_value = strdup(alias->value);
    char **alias_args = NULL;
    int alias_arg_count = 0;
    int alias_arg_capacity = 0;

    // Tokenize the alias value
    char *token = strtok(alias_value, " \t");
    while (token != NULL) {
        // Resize the array if needed
        if (alias_arg_count >= alias_arg_capacity) {
            alias_arg_capacity = alias_arg_capacity == 0 ? 4 : alias_arg_capacity * 2;
            alias_args = realloc(alias_args, alias_arg_capacity * sizeof(char *));
        }

        alias_args[alias_arg_count++] = strdup(token);
        token = strtok(NULL, " \t");
    }

    // Count the original arguments
    int orig_arg_count = 0;
    while (args[orig_arg_count] != NULL) {
        orig_arg_count++;
    }

    // Create a new array for the expanded command
    int total_args = alias_arg_count + orig_arg_count - 1; // -1 because we replace the first arg
    char **expanded_args = malloc((total_args + 1) * sizeof(char *));

    // Copy the alias arguments
    for (int i = 0; i < alias_arg_count; i++) {
        expanded_args[i] = alias_args[i];
    }

    // Copy the remaining original arguments
    for (int i = 1; i < orig_arg_count; i++) {
        expanded_args[alias_arg_count + i - 1] = strdup(args[i]);
    }

    // Null-terminate the array
    expanded_args[total_args] = NULL;

    // Clean up
    free(alias_args);
    free(alias_value);

    return expanded_args;
}

// Initialize aliases
void init_aliases() {
    // You could load aliases from a config file here
    alias_count = 0;

    // Add some default aliases
    add_alias("ll", "ls -l");
    add_alias("la", "ls -a");
    add_alias("l", "ls -CF");
}

// Save aliases to a file
void save_aliases() {
    // Implementation for saving aliases to a config file
}
