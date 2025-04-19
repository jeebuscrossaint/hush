#include "variables.h"

#define MAX_VARIABLES 256

typedef struct {
    char *name;
    char *value;
} ShellVar;

static ShellVar variables[MAX_VARIABLES];
static int var_count = 0;

void init_shell_variables() {
    var_count = 0;

    // Set default POSIX variables
    set_shell_variable("IFS", " \t\n");

    // Set PATH if not already set
    if (getenv("PATH") == NULL) {
        set_shell_variable("PATH", "/usr/local/bin:/usr/bin:/bin");
    }
}

int set_shell_variable(const char *name, const char *value) {
    if (!name || !value) return 0;

    // Check if variable already exists
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            // Update existing variable
            free(variables[i].value);
            variables[i].value = strdup(value);
            return 1;
        }
    }

    // Add new variable if room
    if (var_count < MAX_VARIABLES) {
        variables[var_count].name = strdup(name);
        variables[var_count].value = strdup(value);
        var_count++;
        return 1;
    }

    fprintf(stderr, "hush: too many shell variables\n");
    return 0;
}

char *get_shell_variable(const char *name) {
    if (!name) return NULL;

    // First check shell variables
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return variables[i].value;
        }
    }

    // Then check environment variables
    return getenv(name);
}

int unset_shell_variable(const char *name) {
    if (!name) return 0;

    // Find and remove variable
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            free(variables[i].name);
            free(variables[i].value);

            // Move last variable to this slot
            if (i < var_count - 1) {
                variables[i] = variables[var_count - 1];
            }

            var_count--;
            return 1;
        }
    }

    return 0;
}

// Built-in command: set
int hush_set(char **args) {
    // No arguments: display all variables
    if (args[1] == NULL) {
        // Print all shell variables
        for (int i = 0; i < var_count; i++) {
            printf("%s=%s\n", variables[i].name, variables[i].value);
        }
        return 1;
    }

    // Parse set arguments
    for (int i = 1; args[i] != NULL; i++) {
        // Check for name=value format
        char *equals = strchr(args[i], '=');
        if (equals) {
            *equals = '\0'; // Split at equals sign
            set_shell_variable(args[i], equals + 1);
            *equals = '='; // Restore string
        }
    }

    return 1;
}

// Built-in command: unset
int hush_unset(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "hush: unset: usage: unset NAME...\n");
        return 1;
    }

    for (int i = 1; args[i] != NULL; i++) {
        unset_shell_variable(args[i]);
        unsetenv(args[i]); // Also unset environment variable if it exists
    }

    return 1;
}
