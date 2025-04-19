#include "control.h"
#include "chain.h"
#include "execute.h"
#include "splitline.h"
#include "environment.h"
#include "variables.h"  // Added for set_shell_variable

// Trim leading and trailing whitespace
static char *trim(char *str) {
    if (!str) return NULL;

    // Skip leading spaces
    char *start = str;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    if (*start == '\0') {  // All spaces
        return start;
    }

    // Trim trailing spaces
    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) {
        end--;
    }
    *(end + 1) = '\0';

    return start;
}

// Check if line contains a control structure keyword
int is_control_keyword(const char *line) {
    const char *trimmed = line;

    // Skip leading whitespace
    while (*trimmed && isspace((unsigned char)*trimmed)) {
        trimmed++;
    }

    // Check for control keywords
    if (strncmp(trimmed, "if", 2) == 0 && (isspace(trimmed[2]) || trimmed[2] == '\0')) {
        return 1;
    }
    if (strncmp(trimmed, "then", 4) == 0 && (isspace(trimmed[4]) || trimmed[4] == '\0')) {
        return 2;
    }
    if (strncmp(trimmed, "else", 4) == 0 && (isspace(trimmed[4]) || trimmed[4] == '\0')) {
        return 3;
    }
    if (strncmp(trimmed, "elif", 4) == 0 && (isspace(trimmed[4]) || trimmed[4] == '\0')) {
        return 4;
    }
    if (strncmp(trimmed, "fi", 2) == 0 && (isspace(trimmed[2]) || trimmed[2] == '\0')) {
        return 5;
    }

    return 0;
}

// Check if a line starts a loop
int is_loop_start(const char *line) {
    const char *trimmed = line;

    // Skip leading whitespace
    while (*trimmed && isspace((unsigned char)*trimmed)) {
        trimmed++;
    }

    // Check for loop keywords
    if (strncmp(trimmed, "for", 3) == 0 && (isspace(trimmed[3]) || trimmed[3] == '\0')) {
        return 1; // for loop
    }
    if (strncmp(trimmed, "while", 5) == 0 && (isspace(trimmed[5]) || trimmed[5] == '\0')) {
        return 2; // while loop
    }

    return 0; // not a loop
}

// Check if a line ends a loop
int is_loop_end(const char *line) {
    const char *trimmed = line;

    // Skip leading whitespace
    while (*trimmed && isspace((unsigned char)*trimmed)) {
        trimmed++;
    }

    // Check for loop end keyword
    if (strncmp(trimmed, "done", 4) == 0 && (isspace(trimmed[4]) || trimmed[4] == '\0')) {
        return 1;
    }

    return 0;
}

// Extract the condition part from an if statement
static char *get_condition(const char *if_line) {
    const char *start = if_line;

    // Skip "if" and any spaces
    while (*start && !isspace((unsigned char)*start)) {
        start++;
    }
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    return strdup(start);
}

// Extract items for a for loop
static char **extract_for_items(const char *for_line, int *count) {
    const char *start = for_line;
    char **items = NULL;
    *count = 0;

    // Skip "for" keyword
    while (*start && !isspace((unsigned char)*start)) {
        start++;
    }
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    // Skip variable name
    while (*start && !isspace((unsigned char)*start)) {
        start++;
    }
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    // Skip "in" keyword
    if (strncmp(start, "in", 2) == 0) {
        start += 2;
        while (*start && isspace((unsigned char)*start)) {
            start++;
        }
    }

    // Process items until end of line
    char *items_str = strdup(start);
    if (!items_str) return NULL;

    // Allocate initial array for items
    items = malloc(10 * sizeof(char *));
    if (!items) {
        free(items_str);
        return NULL;
    }

    // Split by whitespace, handling quotes
    char *token = strtok(items_str, " \t\n");
    int capacity = 10;

    while (token != NULL) {
        // Resize array if needed
        if (*count >= capacity) {
            capacity *= 2;
            char **new_items = realloc(items, capacity * sizeof(char *));
            if (!new_items) {
                for (int i = 0; i < *count; i++) {
                    free(items[i]);
                }
                free(items);
                free(items_str);
                return NULL;
            }
            items = new_items;
        }

        // Add item to array
        items[*count] = strdup(token);
        (*count)++;

        token = strtok(NULL, " \t\n");
    }

    free(items_str);
    return items;
}

static char *extract_for_var(const char *for_line) {
    const char *start = for_line;

    // Skip "for" keyword
    while (*start && !isspace((unsigned char)*start)) {
        start++;
    }
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    // Extract variable name
    const char *var_start = start;
    while (*start && !isspace((unsigned char)*start)) {
        start++;
    }

    int var_len = start - var_start;
    char *var_name = malloc(var_len + 1);
    if (!var_name) return NULL;

    strncpy(var_name, var_start, var_len);
    var_name[var_len] = '\0';

    return var_name;
}

// Execute a for loop
int execute_for_loop(char **lines, int line_count) {
    int result = 1; // Success by default
    int do_index = -1;
    int done_index = -1;

    // Find the 'do' and 'done' markers
    for (int i = 1; i < line_count; i++) {
        char *trimmed = trim(lines[i]);
        if (strncmp(trimmed, "do", 2) == 0 && (isspace(trimmed[2]) || trimmed[2] == '\0')) {
            do_index = i;
        } else if (is_loop_end(trimmed)) {
            done_index = i;
            break;
        }
    }

    // Check for syntax errors
    if (do_index == -1) {
        fprintf(stderr, "hush: syntax error: for loop without 'do'\n");
        return 1;
    }
    if (done_index == -1) {
        fprintf(stderr, "hush: syntax error: for loop without 'done'\n");
        return 1;
    }

    // Extract the variable and items
    char *var_name = extract_for_var(lines[0]);
    if (!var_name) {
        fprintf(stderr, "hush: memory allocation error in for loop\n");
        return 1;
    }

    int item_count = 0;
    char **items = extract_for_items(lines[0], &item_count);
    if (!items) {
        fprintf(stderr, "hush: memory allocation error in for loop\n");
        free(var_name);
        return 1;
    }

    // Execute the loop body for each item
    for (int i = 0; i < item_count; i++) {
        // Set the loop variable
        set_shell_variable(var_name, items[i]);

        // Execute the loop body (lines between 'do' and 'done')
        for (int j = do_index + 1; j < done_index; j++) {
            char *cmd = lines[j];
            if (strlen(trim(cmd)) > 0 && !is_control_keyword(cmd)) {
                // Process the command with variable expansion
                char *expanded = expand_variables(cmd);
                result = execute_command_chain(expanded);
                free(expanded);

                // Break on error if needed
                // (optional: you might want to continue the loop regardless of errors)
                // if (result != 0) break;
            }
        }
    }

    // Clean up
    free(var_name);
    for (int i = 0; i < item_count; i++) {
        free(items[i]);
    }
    free(items);

    return result;
}

// Execute a while loop
int execute_while_loop(char **lines, int line_count) {
    int result = 1; // Success by default
    int do_index = -1;
    int done_index = -1;

    // Find the 'do' and 'done' markers
    for (int i = 1; i < line_count; i++) {
        char *trimmed = trim(lines[i]);
        if (strncmp(trimmed, "do", 2) == 0 && (isspace(trimmed[2]) || trimmed[2] == '\0')) {
            do_index = i;
        } else if (is_loop_end(trimmed)) {
            done_index = i;
            break;
        }
    }

    // Check for syntax errors
    if (do_index == -1) {
        fprintf(stderr, "hush: syntax error: while loop without 'do'\n");
        return 1;
    }
    if (done_index == -1) {
        fprintf(stderr, "hush: syntax error: while loop without 'done'\n");
        return 1;
    }

    // Extract the condition (everything after 'while')
    const char *condition_start = lines[0] + 5; // Skip "while"
    while (*condition_start && isspace((unsigned char)*condition_start)) {
        condition_start++;
    }

    char *condition = strdup(condition_start);
    if (!condition) {
        fprintf(stderr, "hush: memory allocation error in while loop\n");
        return 1;
    }

    // Loop execution
    while (1) {
        // Evaluate the condition
        char *expanded_condition = expand_variables(condition);
        int condition_result = execute_command_chain(expanded_condition);
        free(expanded_condition);

        // If the condition is false (non-zero exit status), break the loop
        if (condition_result != 0) {
            break;
        }

        // Execute the loop body
        for (int j = do_index + 1; j < done_index; j++) {
            char *cmd = lines[j];
            if (strlen(trim(cmd)) > 0 && !is_control_keyword(cmd)) {
                // Process the command with variable expansion
                char *expanded = expand_variables(cmd);
                result = execute_command_chain(expanded);
                free(expanded);

                // Could handle break/continue here if we implement them
            }
        }
    }

    free(condition);
    return result;
}

// Execute an if-then-else-fi block
int execute_if_statement(char **lines, int line_count) {
    int i = 0;
    int if_result = 0;
    int in_then_block = 0;
    int in_else_block = 0;
    int else_index = -1;
    int fi_index = -1;

    // First, find the structure of the if statement
    for (i = 0; i < line_count; i++) {
        int keyword = is_control_keyword(lines[i]);

        if (keyword == 2) {  // then
            in_then_block = 1;
        } else if (keyword == 3) {  // else
            in_then_block = 0;
            in_else_block = 1;
            else_index = i;
        } else if (keyword == 5) {  // fi
            fi_index = i;
            break;
        }
    }

    // If we didn't find a complete structure, report an error
    if (fi_index == -1) {
        fprintf(stderr, "hush: syntax error: if without fi\n");
        return 1;
    }

    // Execute the condition
    char *condition = get_condition(lines[0]);
    if_result = execute_command_chain(condition);
    free(condition);

    // Find where the "then" block starts
    int then_start = -1;
    for (i = 1; i < line_count; i++) {
        if (is_control_keyword(lines[i]) == 2) {  // then
            then_start = i + 1;
            break;
        }
    }

    if (then_start == -1 || then_start >= line_count) {
        fprintf(stderr, "hush: syntax error: if without then\n");
        return 1;
    }

    // Execute the appropriate block based on the condition result
    int block_end = else_index != -1 ? else_index : fi_index;
    int result = 1;  // Default success

    if (if_result == 0) {  // Condition succeeded (zero exit status)
        // Execute the then block
        for (i = then_start; i < block_end; i++) {
            if (is_control_keyword(lines[i])) continue;  // Skip embedded keywords
            result = execute_command_chain(lines[i]);
        }
    } else if (else_index != -1) {  // Condition failed and there's an else block
        // Execute the else block
        for (i = else_index + 1; i < fi_index; i++) {
            if (is_control_keyword(lines[i])) continue;  // Skip embedded keywords
            result = execute_command_chain(lines[i]);
        }
    }

    return result;
}

// Parse and execute a series of lines that may contain control structures
int parse_and_execute_control(char **lines, int line_count) {
    int i = 0;
    int result = 1;  // Default success

    while (i < line_count) {
        // Check for control structures
        if (is_control_keyword(lines[i]) == 1) {  // if statement
            // Find matching fi
            int nesting = 1;
            int fi_index = -1;

            for (int j = i + 1; j < line_count; j++) {
                int kw = is_control_keyword(lines[j]);
                if (kw == 1) {  // Nested if
                    nesting++;
                } else if (kw == 5) {  // fi
                    nesting--;
                    if (nesting == 0) {
                        fi_index = j;
                        break;
                    }
                }
            }

            if (fi_index == -1) {
                fprintf(stderr, "hush: syntax error: if without fi\n");
                return 1;
            }

            // Create a subarray with just the if block
            char **if_block = malloc((fi_index - i + 1) * sizeof(char *));
            if (!if_block) {
                perror("hush: memory allocation error");
                return 1;
            }

            for (int j = i; j <= fi_index; j++) {
                if_block[j - i] = lines[j];
            }

            // Execute the if statement
            result = execute_if_statement(if_block, fi_index - i + 1);

            // Clean up and move past this block
            free(if_block);
            i = fi_index + 1;
        }
        else if (is_loop_start(lines[i]) == 1) {  // for loop
            // Find matching done
            int nesting = 1;
            int done_index = -1;

            for (int j = i + 1; j < line_count; j++) {
                if (is_loop_start(lines[j])) {
                    nesting++;
                } else if (is_loop_end(lines[j])) {
                    nesting--;
                    if (nesting == 0) {
                        done_index = j;
                        break;
                    }
                }
            }

            if (done_index == -1) {
                fprintf(stderr, "hush: syntax error: for without done\n");
                return 1;
            }

            // Create a subarray with just the for block
            char **for_block = malloc((done_index - i + 1) * sizeof(char *));
            if (!for_block) {
                perror("hush: memory allocation error");
                return 1;
            }

            for (int j = i; j <= done_index; j++) {
                for_block[j - i] = lines[j];
            }

            // Execute the for loop
            result = execute_for_loop(for_block, done_index - i + 1);

            // Clean up and move past this block
            free(for_block);
            i = done_index + 1;
        }
        else if (is_loop_start(lines[i]) == 2) {  // while loop
            // Find matching done
            int nesting = 1;
            int done_index = -1;

            for (int j = i + 1; j < line_count; j++) {
                if (is_loop_start(lines[j])) {
                    nesting++;
                } else if (is_loop_end(lines[j])) {
                    nesting--;
                    if (nesting == 0) {
                        done_index = j;
                        break;
                    }
                }
            }

            if (done_index == -1) {
                fprintf(stderr, "hush: syntax error: while without done\n");
                return 1;
            }

            // Create a subarray with just the while block
            char **while_block = malloc((done_index - i + 1) * sizeof(char *));
            if (!while_block) {
                perror("hush: memory allocation error");
                return 1;
            }

            for (int j = i; j <= done_index; j++) {
                while_block[j - i] = lines[j];
            }

            // Execute the while loop
            result = execute_while_loop(while_block, done_index - i + 1);

            // Clean up and move past this block
            free(while_block);
            i = done_index + 1;
        }
        else {
            // Regular command, execute it
            if (strlen(trim(lines[i])) > 0) {
                char *expanded = expand_variables(lines[i]);
                result = execute_command_chain(expanded);
                free(expanded);
            }
            i++;
        }
    }

    return result;
}

// Process a script file, handling control structures
int process_script_control_flow(FILE *script_file) {
    char line[1024];
    char **lines = NULL;
    int line_count = 0;
    int capacity = 16;
    int result = 1;

    // Allocate initial array for lines
    lines = malloc(capacity * sizeof(char *));
    if (!lines) {
        perror("hush: memory allocation error");
        return 1;
    }

    // Read all lines from the script
    while (fgets(line, sizeof(line), script_file)) {
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        // Skip comments and empty lines
        char *trimmed = trim(line);
        if (trimmed[0] == '#' || trimmed[0] == '\0') {
            continue;
        }

        // Expand the array if needed
        if (line_count >= capacity) {
            capacity *= 2;
            char **new_lines = realloc(lines, capacity * sizeof(char *));
            if (!new_lines) {
                perror("hush: memory allocation error");
                for (int i = 0; i < line_count; i++) {
                    free(lines[i]);
                }
                free(lines);
                return 1;
            }
            lines = new_lines;
        }

        // Add this line to the array
        lines[line_count++] = strdup(line);
    }

    // Parse and execute the script
    result = parse_and_execute_control(lines, line_count);

    // Clean up
    for (int i = 0; i < line_count; i++) {
        free(lines[i]);
    }
    free(lines);

    return result;
}
