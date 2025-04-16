#include "redirection.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Function to check if a token is a redirection operator
int is_redirection(char *token) {
    return (strcmp(token, "<") == 0 ||
            strcmp(token, ">") == 0 ||
            strcmp(token, ">>") == 0 ||
            strcmp(token, "2>") == 0 ||
            strcmp(token, "2>>") == 0 ||
            strcmp(token, "&>") == 0 ||
            strcmp(token, "<<") == 0);
}

// Setup redirection based on command arguments
// Returns new args array with redirection operators removed
char **setup_redirection(char **args, int *stdin_copy, int *stdout_copy, int *stderr_copy) {
    // Make copies of the standard file descriptors
    *stdin_copy = dup(STDIN_FILENO);
    *stdout_copy = dup(STDOUT_FILENO);
    *stderr_copy = dup(STDERR_FILENO);

    if (*stdin_copy == -1 || *stdout_copy == -1 || *stderr_copy == -1) {
        perror("hush: dup error");
        return args;
    }

    // Count number of arguments
    int argc = 0;
    while (args[argc] != NULL) {
        argc++;
    }

    // Allocate new array for cleaned arguments (without redirection)
    char **new_args = malloc((argc + 1) * sizeof(char *));
    if (new_args == NULL) {
        perror("hush: allocation error");
        return args;
    }

    int new_argc = 0;
    int here_doc = 0;  // Flag to indicate if we're processing a here document
    char *delimiter = NULL;  // Delimiter for here document
    char *here_doc_content = NULL;  // Content of here document

    // Process all arguments, looking for redirection operators
    for (int i = 0; i < argc; i++) {
        if (is_redirection(args[i])) {
            // It's a redirection operator
            if (i + 1 < argc) { // Make sure there's a filename/delimiter after
                int fd; // File descriptor for the redirection

                if (strcmp(args[i], "<") == 0) {
                    // Input redirection
                    fd = open(args[i+1], O_RDONLY);
                    if (fd == -1) {
                        perror("hush: input redirection error");
                        break;
                    }
                    if (dup2(fd, STDIN_FILENO) == -1) {
                        perror("hush: dup2 error");
                        close(fd);
                        break;
                    }
                    close(fd);
                }
                else if (strcmp(args[i], ">") == 0) {
                    // Output redirection (truncate)
                    fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd == -1) {
                        perror("hush: output redirection error");
                        break;
                    }
                    if (dup2(fd, STDOUT_FILENO) == -1) {
                        perror("hush: dup2 error");
                        close(fd);
                        break;
                    }
                    close(fd);
                }
                else if (strcmp(args[i], ">>") == 0) {
                    // Output redirection (append)
                    fd = open(args[i+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
                    if (fd == -1) {
                        perror("hush: output redirection error");
                        break;
                    }
                    if (dup2(fd, STDOUT_FILENO) == -1) {
                        perror("hush: dup2 error");
                        close(fd);
                        break;
                    }
                    close(fd);
                }
                else if (strcmp(args[i], "2>") == 0) {
                    // Error redirection (truncate)
                    fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd == -1) {
                        perror("hush: error redirection error");
                        break;
                    }
                    if (dup2(fd, STDERR_FILENO) == -1) {
                        perror("hush: dup2 error");
                        close(fd);
                        break;
                    }
                    close(fd);
                }
                else if (strcmp(args[i], "2>>") == 0) {
                    // Error redirection (append)
                    fd = open(args[i+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
                    if (fd == -1) {
                        perror("hush: error redirection error");
                        break;
                    }
                    if (dup2(fd, STDERR_FILENO) == -1) {
                        perror("hush: dup2 error");
                        close(fd);
                        break;
                    }
                    close(fd);
                }
                else if (strcmp(args[i], "&>") == 0) {
                    // Combined stdout and stderr redirection
                    fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd == -1) {
                        perror("hush: output redirection error");
                        break;
                    }
                    // Redirect both stdout and stderr to the same file
                    if (dup2(fd, STDOUT_FILENO) == -1 || dup2(fd, STDERR_FILENO) == -1) {
                        perror("hush: dup2 error");
                        close(fd);
                        break;
                    }
                    close(fd);
                }
                else if (strcmp(args[i], "<<") == 0) {
                    // Here document
                    delimiter = args[i+1];
                    here_doc = 1;

                    // Create a temporary file for the here document
                    char temp_filename[] = "/tmp/hush_heredoc_XXXXXX";
                    fd = mkstemp(temp_filename);
                    if (fd == -1) {
                        perror("hush: here document error");
                        break;
                    }

                    // Remove the file so it's automatically cleaned up
                    unlink(temp_filename);

                    // Prompt and read lines until the delimiter is found
                    printf("heredoc> ");
                    fflush(stdout);

                    char line[1024];
                    while (fgets(line, sizeof(line), stdin) != NULL) {
                        // Remove the trailing newline
                        size_t len = strlen(line);
                        if (len > 0 && line[len-1] == '\n') {
                            line[len-1] = '\0';
                            len--;
                        }

                        // Check if this line is the delimiter
                        if (strcmp(line, delimiter) == 0) {
                            break;
                        }

                        // Write the line to the temp file
                        // Add back the newline for the temp file
                        line[len] = '\n';
                        line[len+1] = '\0';
                        if (write(fd, line, len+1) == -1) {
                            perror("hush: here document write error");
                            close(fd);
                            break;
                        }

                        printf("heredoc> ");
                        fflush(stdout);
                    }

                    // Rewind the file to the beginning
                    if (lseek(fd, 0, SEEK_SET) == -1) {
                        perror("hush: here document lseek error");
                        close(fd);
                        break;
                    }

                    // Redirect stdin from this temp file
                    if (dup2(fd, STDIN_FILENO) == -1) {
                        perror("hush: dup2 error");
                        close(fd);
                        break;
                    }
                    close(fd);
                }

                // Skip the filename/delimiter token
                i++;
            } else {
                fprintf(stderr, "hush: syntax error near unexpected token `%s'\n", args[i]);
                break;
            }
        } else {
            // Regular argument, copy to new args array
            new_args[new_argc++] = args[i];
        }
    }

    // Null-terminate the new args array
    new_args[new_argc] = NULL;

    return new_args;
}

// Reset file descriptors after a command completes
void reset_redirection(int stdin_copy, int stdout_copy, int stderr_copy) {
    if (stdin_copy != -1) {
        dup2(stdin_copy, STDIN_FILENO);
        close(stdin_copy);
    }

    if (stdout_copy != -1) {
        dup2(stdout_copy, STDOUT_FILENO);
        close(stdout_copy);
    }

    if (stderr_copy != -1) {
        dup2(stderr_copy, STDERR_FILENO);
        close(stderr_copy);
    }
}
