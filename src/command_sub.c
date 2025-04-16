#include "command_sub.h"

// Check if a string contains command substitution
int has_command_substitution(const char *line) {
    return (strchr(line, '`') != NULL ||
            (strchr(line, '$') != NULL && strchr(line, '(') != NULL));
}

// Execute a command and capture its output
char *capture_command_output(const char *command) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("hush: pipe error in command substitution");
        return strdup("");
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("hush: fork error in command substitution");
        close(pipefd[0]);
        close(pipefd[1]);
        return strdup("");
    }

    if (pid == 0) {
        // Child process
        close(pipefd[0]);  // Close read end

        // Redirect stdout to pipe
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("hush: dup2 error in command substitution");
            exit(EXIT_FAILURE);
        }

        // Also redirect stderr to pipe (optional)
        if (dup2(pipefd[1], STDERR_FILENO) == -1) {
            perror("hush: dup2 error redirecting stderr");
            exit(EXIT_FAILURE);
        }

        close(pipefd[1]);  // Close the pipe's write end in child

        // Execute the command through the shell
        execl("/bin/sh", "sh", "-c", command, (char *)NULL);

        // If execl fails
        perror("hush: exec error in command substitution");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        close(pipefd[1]);  // Close write end

        // Read the command's output
        char buffer[4096];
        size_t total_size = 0;
        size_t buffer_size = 4096;
        char *output = malloc(buffer_size);
        if (!output) {
            perror("hush: malloc error in command substitution");
            close(pipefd[0]);
            return strdup("");
        }
        output[0] = '\0';

        ssize_t bytes_read;
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';

            // Check if we need to resize the output buffer
            if (total_size + bytes_read + 1 > buffer_size) {
                buffer_size = (total_size + bytes_read + 1) * 2;
                output = realloc(output, buffer_size);
                if (!output) {
                    perror("hush: realloc error in command substitution");
                    close(pipefd[0]);
                    return strdup("");
                }
            }

            // Append the read data to the output
            strcat(output + total_size, buffer);
            total_size += bytes_read;
        }

        close(pipefd[0]);  // Close read end

        // Wait for the child to complete
        int status;
        waitpid(pid, &status, 0);

        // Remove trailing newlines
        while (total_size > 0 && (output[total_size-1] == '\n' || output[total_size-1] == '\r')) {
            output[--total_size] = '\0';
        }

        return output;
    }
}

// Helper function to find matching parenthesis
static int find_matching_parenthesis(const char *str, int start) {
    int depth = 1;
    int i = start;

    while (str[i] != '\0' && depth > 0) {
        if (str[i] == '(') {
            depth++;
        } else if (str[i] == ')') {
            depth--;
        }
        i++;
    }

    if (depth != 0) return -1;  // No matching parenthesis
    return i - 1;
}

// Helper function to find matching backtick
static int find_matching_backtick(const char *str, int start) {
    int i = start;

    while (str[i] != '\0') {
        if (str[i] == '`' && (i == 0 || str[i-1] != '\\')) {
            return i;
        }
        i++;
    }

    return -1;  // No matching backtick
}

// Process command substitution for the entire line
char *perform_command_substitution(char *line) {
    if (!has_command_substitution(line)) {
        return strdup(line);
    }

    // Allocate a reasonably large initial buffer
    size_t initial_size = strlen(line) * 4 + 1024;
    size_t result_size = initial_size;
    char *result = malloc(result_size);
    if (!result) {
        perror("hush: malloc error in command substitution");
        return strdup(line);
    }
    result[0] = '\0';

    int i = 0;
    int len = strlen(line);
    int result_pos = 0;

    while (i < len) {
        // Check for $(command) substitution
        if (line[i] == '$' && i + 1 < len && line[i+1] == '(') {
            int end = find_matching_parenthesis(line, i + 2);
            if (end != -1) {
                // Extract the command
                int cmd_len = end - (i + 2);
                char *cmd = malloc(cmd_len + 1);
                if (!cmd) {
                    perror("hush: malloc error");
                    free(result);
                    return strdup(line);
                }
                strncpy(cmd, line + i + 2, cmd_len);
                cmd[cmd_len] = '\0';

                // Execute the command and get its output
                char *output = capture_command_output(cmd);
                size_t output_len = strlen(output);

                // Make sure there's enough space in result
                if (result_pos + output_len + 1 > result_size) {
                    // Double the result size or ensure it's big enough for this output
                    size_t new_size = (result_size * 2 > result_pos + output_len + 1) ?
                                       result_size * 2 : result_pos + output_len + 1024;
                    char *new_result = realloc(result, new_size);
                    if (!new_result) {
                        perror("hush: realloc error");
                        free(cmd);
                        free(output);
                        free(result);
                        return strdup(line);
                    }
                    result = new_result;
                    result_size = new_size;
                }

                // Append the output to the result (safely)
                memcpy(result + result_pos, output, output_len + 1);
                result_pos += output_len;

                // Clean up and move past the command substitution
                free(cmd);
                free(output);
                i = end + 1;
                continue;
            }
        }

        // Check for `command` substitution
        if (line[i] == '`' && (i == 0 || line[i-1] != '\\')) {
            int end = find_matching_backtick(line, i + 1);
            if (end != -1) {
                // Extract the command
                int cmd_len = end - (i + 1);
                char *cmd = malloc(cmd_len + 1);
                if (!cmd) {
                    perror("hush: malloc error");
                    free(result);
                    return strdup(line);
                }
                strncpy(cmd, line + i + 1, cmd_len);
                cmd[cmd_len] = '\0';

                // Execute the command and get its output
                char *output = capture_command_output(cmd);
                size_t output_len = strlen(output);

                // Make sure there's enough space in result
                if (result_pos + output_len + 1 > result_size) {
                    // Double the result size or ensure it's big enough for this output
                    size_t new_size = (result_size * 2 > result_pos + output_len + 1) ?
                                       result_size * 2 : result_pos + output_len + 1024;
                    char *new_result = realloc(result, new_size);
                    if (!new_result) {
                        perror("hush: realloc error");
                        free(cmd);
                        free(output);
                        free(result);
                        return strdup(line);
                    }
                    result = new_result;
                    result_size = new_size;
                }

                // Append the output to the result (safely)
                memcpy(result + result_pos, output, output_len + 1);
                result_pos += output_len;

                // Clean up and move past the command substitution
                free(cmd);
                free(output);
                i = end + 1;
                continue;
            }
        }

        // Regular character - ensure we have enough space
        if (result_pos + 2 > result_size) {
            result_size *= 2;
            char *new_result = realloc(result, result_size);
            if (!new_result) {
                perror("hush: realloc error");
                free(result);
                return strdup(line);
            }
            result = new_result;
        }

        result[result_pos++] = line[i++];
    }

    result[result_pos] = '\0';

    // Check if we need to process nested substitutions
    if (has_command_substitution(result)) {
        char *nested_result = perform_command_substitution(result);
        free(result);
        return nested_result;
    }

    // Reallocate to exact size to save memory
    char *final_result = strdup(result);
    free(result);
    return final_result;
}
