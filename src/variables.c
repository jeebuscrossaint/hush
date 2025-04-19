#include "variables.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_VARIABLES 256

typedef struct {
    char *name;
    char *value;
} ShellVar;

static ShellVar variables[MAX_VARIABLES];
static int var_count = 0;

// Special value trackers
static int last_exit_status = 0;
static pid_t last_background_pid = 0;
static char **script_args = NULL;
static int script_arg_count = 0;

void init_shell_variables() {
    var_count = 0;

    // Set default POSIX variables
    set_shell_variable("IFS", " \t\n");

    // Set PATH if not already set
    if (getenv("PATH") == NULL) {
        set_shell_variable("PATH", "/usr/local/bin:/usr/bin:/bin");
    }

    // Initialize special variables
    last_exit_status = 0;
    last_background_pid = 0;
    script_args = NULL;
    script_arg_count = 0;
}

// Set or update a shell variable
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

// Get the value of a shell variable
char *get_shell_variable(const char *name) {
    if (!name) return NULL;

    // Check for special variables first
    if (strcmp(name, "?") == 0) {
        char *result = malloc(16);
        if (result) sprintf(result, "%d", last_exit_status);
        return result;
    }
    else if (strcmp(name, "$") == 0) {
        char *result = malloc(16);
        if (result) sprintf(result, "%d", (int)getpid());
        return result;
    }
    else if (strcmp(name, "!") == 0) {
        char *result = malloc(16);
        if (result) sprintf(result, "%d", (int)last_background_pid);
        return result;
    }
    else if (strcmp(name, "#") == 0) {
        char *result = malloc(16);
        if (result) sprintf(result, "%d", script_arg_count);
        return result;
    }
    else if (isdigit(name[0])) {
        // Positional parameter
        int index = atoi(name);
        if (index >= 0 && index <= script_arg_count) {
            return script_args[index] ? strdup(script_args[index]) : strdup("");
        }
        return strdup("");
    }

    // Check shell variables
    for (int i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return strdup(variables[i].value);
        }
    }

    // Then check environment variables
    char *env_value = getenv(name);
    return env_value ? strdup(env_value) : NULL;
}

// Remove a shell variable
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

// Set the last exit status
void set_last_exit_status(int status) {
    last_exit_status = status;
}

// Get the last exit status
int get_last_exit_status() {
    return last_exit_status;
}

// Set last background process PID
void set_last_background_pid(pid_t pid) {
    last_background_pid = pid;
}

// Get last background process PID
pid_t get_last_background_pid() {
    return last_background_pid;
}

// Set script arguments
void set_script_args(int argc, char **argv) {
    // Free any existing arguments
    if (script_args) {
        for (int i = 0; i < script_arg_count; i++) {
            free(script_args[i]);
        }
        free(script_args);
    }

    // Set new arguments
    script_arg_count = argc;
    if (argc > 0) {
        script_args = malloc((argc + 1) * sizeof(char *));
        if (script_args) {
            for (int i = 0; i < argc; i++) {
                script_args[i] = strdup(argv[i]);
            }
            script_args[argc] = NULL;
        } else {
            script_arg_count = 0;
            perror("hush: malloc error in set_script_args");
        }
    } else {
        script_args = NULL;
    }
}

// Get script argument count
int get_script_arg_count() {
    return script_arg_count;
}

// Get specific script argument
char *get_script_arg(int index) {
    if (index >= 0 && index <= script_arg_count && script_args) {
        return script_args[index] ? strdup(script_args[index]) : NULL;
    }
    return NULL;
}

// Helper function to parse parameter expansion arguments
static char *parse_expansion_arg(const char *param, int *offset) {
    char buffer[1024] = {0};
    int buf_pos = 0;
    int brace_level = 0;

    while (param[*offset]) {
        if (param[*offset] == '}' && brace_level == 0) {
            break;
        } else if (param[*offset] == '{') {
            brace_level++;
            buffer[buf_pos++] = param[(*offset)++];
        } else if (param[*offset] == '}') {
            brace_level--;
            buffer[buf_pos++] = param[(*offset)++];
        } else {
            buffer[buf_pos++] = param[(*offset)++];
        }
    }

    // Null-terminate
    buffer[buf_pos] = '\0';
    return strdup(buffer);
}

// Handle advanced parameter expansion
char *expand_parameter(const char *param, int *offset) {
    char var_name[256] = {0};
    int var_name_len = 0;
    char *default_value = NULL;
    int op_type = 0;  // 0: normal, 1: :-, 2: :=, 3: :+, 4: :?, 5: #, 6: ##, etc.

    // Skip the ${
    *offset += 2;

    // Parse the variable name and operator
    while (param[*offset] && !isspace(param[*offset])) {
        if (param[*offset] == '}') {
            // Simple ${var} form
            var_name[var_name_len] = '\0';
            (*offset)++;  // Skip }
            return get_shell_variable(var_name);
        } else if (param[*offset] == ':' && param[*offset + 1] == '-') {
            // ${var:-default}
            var_name[var_name_len] = '\0';
            *offset += 2;  // Skip :-
            op_type = 1;
            default_value = parse_expansion_arg(param, offset);
            break;
        } else if (param[*offset] == ':' && param[*offset + 1] == '=') {
            // ${var:=default}
            var_name[var_name_len] = '\0';
            *offset += 2;  // Skip :=
            op_type = 2;
            default_value = parse_expansion_arg(param, offset);
            break;
        } else if (param[*offset] == ':' && param[*offset + 1] == '+') {
            // ${var:+value}
            var_name[var_name_len] = '\0';
            *offset += 2;  // Skip :+
            op_type = 3;
            default_value = parse_expansion_arg(param, offset);
            break;
        } else if (param[*offset] == ':' && param[*offset + 1] == '?') {
            // ${var:?message}
            var_name[var_name_len] = '\0';
            *offset += 2;  // Skip :?
            op_type = 4;
            default_value = parse_expansion_arg(param, offset);
            break;
        } else if (param[*offset] == '#' && param[*offset + 1] != '#') {
            // ${#var} - length of var
            var_name[var_name_len] = '\0';
            *offset += 1;  // Skip #
            op_type = 5;
            break;
        } else {
            // Part of variable name
            var_name[var_name_len++] = param[(*offset)++];
        }
    }

    if (param[*offset] == '}') {
        (*offset)++; // Skip closing brace
    }

    // Get variable value
    char *var_value = get_shell_variable(var_name);

    // Process according to operator
    switch (op_type) {
        case 0:  // ${var} - simple variable
            return var_value ? var_value : strdup("");

        case 1:  // ${var:-default}
            if (var_value && *var_value) {
                free(default_value);
                return var_value;
            }
            free(var_value);
            return default_value ? default_value : strdup("");

        case 2:  // ${var:=default}
            if (var_value && *var_value) {
                free(default_value);
                return var_value;
            }
            free(var_value);
            set_shell_variable(var_name, default_value ? default_value : "");
            return default_value ? strdup(default_value) : strdup("");

        case 3:  // ${var:+value}
            if (var_value && *var_value) {
                free(var_value);
                return default_value ? default_value : strdup("");
            }
            free(var_value);
            free(default_value);
            return strdup("");

        case 4:  // ${var:?message}
            if (var_value && *var_value) {
                free(default_value);
                return var_value;
            }
            // Print error message and fail
            fprintf(stderr, "hush: %s: %s\n", var_name,
                    default_value ? default_value : "parameter null or not set");
            free(var_value);
            free(default_value);
            return strdup("");

        case 5:  // ${#var} - length
            {
                if (var_value) {
                    int len = strlen(var_value);
                    free(var_value);
                    char *result = malloc(16);
                    if (result) sprintf(result, "%d", len);
                    return result;
                }
                return strdup("0");
            }

        default:
            free(var_value);
            free(default_value);
            return strdup("");
    }
}

// Improved variable expansion to handle all types of variables
char *expand_variables(char *line) {
    if (!line) return NULL;

    // First pass: calculate size needed for expansion
    size_t expanded_size = 0;
    for (char *p = line; *p; p++) {
        if (*p == '$') {
            if (*(p+1) == '{') {
                // Complex variable, e.g. ${VAR}
                char *close_brace = strchr(p+2, '}');
                if (close_brace) {
                    int offset = 0;
                    char *value = expand_parameter(p, &offset);
                    if (value) {
                        expanded_size += strlen(value);
                        free(value);
                    }
                    p = p + offset - 1;  // -1 because of p++ in loop
                } else {
                    // Malformed ${, treat as literal
                    expanded_size += 2;
                    p++;
                }
            } else if (isalnum(*(p+1)) || *(p+1) == '_' ||
                      *(p+1) == '?' || *(p+1) == '$' ||
                      *(p+1) == '!' || *(p+1) == '#' ||
                      isdigit(*(p+1))) {
                // Simple variable or special variable
                char var_name[256] = {0};
                int i = 0;
                p++;  // Skip $

                // Extract variable name
                while (isalnum(*p) || *p == '_' || *p == '?' ||
                       *p == '$' || *p == '!' || *p == '#' || isdigit(*p)) {
                    var_name[i++] = *p++;
                }
                p--;  // Step back one as the loop will p++

                // Get variable value
                char *value = get_shell_variable(var_name);
                if (value) {
                    expanded_size += strlen(value);
                    free(value);
                }
            } else {
                // Literal $ (not followed by valid variable start)
                expanded_size++;
            }
        } else {
            expanded_size++;
        }
    }

    // Allocate result
    char *result = malloc(expanded_size + 1);  // +1 for null terminator
    if (!result) {
        perror("hush: malloc error in expand_variables");
        return strdup(line);
    }

    // Second pass: perform expansion
    char *dest = result;
    for (char *p = line; *p; p++) {
        if (*p == '$') {
            if (*(p+1) == '{') {
                // Complex variable, e.g. ${VAR}
                char *close_brace = strchr(p+2, '}');
                if (close_brace) {
                    int offset = 0;
                    char *value = expand_parameter(p, &offset);
                    if (value) {
                        strcpy(dest, value);
                        dest += strlen(value);
                        free(value);
                    }
                    p = p + offset - 1;  // -1 because of p++ in loop
                } else {
                    // Malformed ${, treat as literal
                    *dest++ = *p++;
                    *dest++ = *p;
                }
            } else if (isalnum(*(p+1)) || *(p+1) == '_' ||
                      *(p+1) == '?' || *(p+1) == '$' ||
                      *(p+1) == '!' || *(p+1) == '#' ||
                      isdigit(*(p+1))) {
                // Simple variable or special variable
                char var_name[256] = {0};
                int i = 0;
                p++;  // Skip $

                // Extract variable name
                while (isalnum(*p) || *p == '_' || *p == '?' ||
                       *p == '$' || *p == '!' || *p == '#' || isdigit(*p)) {
                    var_name[i++] = *p++;
                }
                p--;  // Step back one as the loop will p++

                // Get variable value
                char *value = get_shell_variable(var_name);
                if (value) {
                    strcpy(dest, value);
                    dest += strlen(value);
                    free(value);
                }
            } else {
                // Literal $ (not followed by valid variable start)
                *dest++ = *p;
            }
        } else {
            *dest++ = *p;
        }
    }

    *dest = '\0';  // Null-terminate

    return result;
}

// Built-in: set
int hush_set(char **args) {
    // No arguments: display all variables
    if (args[1] == NULL) {
        // Print all shell variables
        for (int i = 0; i < var_count; i++) {
            printf("%s=%s\n", variables[i].name, variables[i].value);
        }
        return 1;
    }

    // Check for flags
    if (args[1][0] == '-') {
        // Process shell option flags
        for (int i = 1; args[1][i] != '\0'; i++) {
            switch (args[1][i]) {
                case 'x':
                    // Set xtrace mode
                    set_shell_variable("XTRACE", "1");
                    break;
                case 'e':
                    // Set errexit mode
                    set_shell_variable("ERREXIT", "1");
                    break;
                case 'u':
                    // Set nounset mode
                    set_shell_variable("NOUNSET", "1");
                    break;
                // Add more options as needed
                default:
                    fprintf(stderr, "hush: set: unknown option: -%c\n", args[1][i]);
            }
        }
        return 1;
    }

    // Parse name=value arguments
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

// Built-in: unset
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

// Built-in: shift
int hush_shift(char **args) {
    int n = 1;  // Default shift count is 1

    if (args[1]) {
        n = atoi(args[1]);
        if (n < 0) {
            fprintf(stderr, "hush: shift: %s: shift count must be >= 0\n", args[1]);
            return 1;
        }
    }

    if (n > script_arg_count) {
        fprintf(stderr, "hush: shift: shift count must be <= %d\n", script_arg_count);
        return 1;
    }

    if (n == 0) return 1;  // Nothing to do

    // Shift arguments
    for (int i = n; i <= script_arg_count; i++) {
        free(script_args[i - n]);
        script_args[i - n] = (i < script_arg_count) ? strdup(script_args[i]) : NULL;
    }

    script_arg_count -= n;

    return 1;
}
