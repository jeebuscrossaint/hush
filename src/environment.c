#include "environment.h"
#include <ctype.h>
#include "variables.h"

/*char *expand_variables(char *line) {
    if (line == NULL) {
        return NULL;
    }

    // First, calculate how much space we'll need
    size_t expanded_size = 0;
    char *temp = line;

    while (*temp) {
        if (*temp == '$') {
            // Potential variable
            if (isalnum(*(temp + 1)) || *(temp + 1) == '_') {
                // $VAR format
                temp++; // Skip $
                char var_name[256] = {0};
                size_t i = 0;

                while ((isalnum(*temp) || *temp == '_') && i < 255) {
                    var_name[i++] = *temp++;
                }
                var_name[i] = '\0';

                char *var_value = getenv(var_name);
                if (var_value) {
                    expanded_size += strlen(var_value);
                }
            } else if (*(temp + 1) == '{') {
                // ${VAR} format
                temp += 2; // Skip ${
                char var_name[256] = {0};
                size_t i = 0;

                while (*temp && *temp != '}' && i < 255) {
                    var_name[i++] = *temp++;
                }
                var_name[i] = '\0';

                if (*temp == '}') temp++; // Skip }

                char *var_value = getenv(var_name);
                if (var_value) {
                    expanded_size += strlen(var_value);
                }
            } else {
                // Just a $ character
                expanded_size++;
                temp++;
            }
        } else {
            // Regular character
            expanded_size++;
            temp++;
        }
    }

    // Allocate enough space plus one for null terminator
    char *expanded = malloc(expanded_size + 1);
    if (!expanded) {
        perror("hush: allocation error in expand_variables");
        return strdup(line); // Return original if allocation fails
    }

    // Now do the actual expansion
    char *dest = expanded;
    char *src = line;

    while (*src) {
        if (*src == '$') {
            if (isalnum(*(src + 1)) || *(src + 1) == '_') {
                // $VAR format
                src++; // Skip $
                char var_name[256] = {0};
                size_t i = 0;

                while ((isalnum(*src) || *src == '_') && i < 255) {
                    var_name[i++] = *src++;
                }
                var_name[i] = '\0';

                char *var_value = getenv(var_name);
                if (var_value) {
                    strcpy(dest, var_value);
                    dest += strlen(var_value);
                }
            } else if (*(src + 1) == '{') {
                // ${VAR} format
                src += 2; // Skip ${
                char var_name[256] = {0};
                size_t i = 0;

                while (*src && *src != '}' && i < 255) {
                    var_name[i++] = *src++;
                }
                var_name[i] = '\0';

                if (*src == '}') src++; // Skip }

                char *var_value = getenv(var_name);
                if (var_value) {
                    strcpy(dest, var_value);
                    dest += strlen(var_value);
                }
            } else {
                // Just a $ character
                *dest++ = *src++;
            }
        } else {
            // Regular character
            *dest++ = *src++;
        }
    }

    *dest = '\0'; // Null terminate the result
    return expanded;
    }*/

int hush_export(char **args) {
    // The 'export' built-in command
    if (args[1] == NULL) {
        // With no arguments, list all environment variables
        extern char **environ;
        for (char **env = environ; *env != NULL; env++) {
            printf("%s\n", *env);
        }
        return 1;
    }

    // Process each argument
    for (int i = 1; args[i] != NULL; i++) {
        char *arg = args[i];
        char *equals = strchr(arg, '=');

        if (equals == NULL) {
            // No equals sign, print the current value
            char *value = getenv(arg);
            if (value != NULL) {
                printf("%s=%s\n", arg, value);
            }
            continue;
        }

        // Split into name and value
        *equals = '\0'; // Temporarily null-terminate the name
        char *name = arg;
        char *value = equals + 1;

        // Set the environment variable
        if (setenv(name, value, 1) != 0) {
            fprintf(stderr, "hush: export: error setting environment variable %s\n", name);
        }

        *equals = '='; // Restore the equals sign
    }

    return 1;
}
