#include "dir_stack.h"

#define MAX_DIR_STACK 100

// The directory stack
static char *dir_stack[MAX_DIR_STACK];
static int stack_size = 0;

// Initialize the directory stack
void init_dir_stack(void) {
    stack_size = 0;
}

// Clean up the directory stack
void free_dir_stack(void) {
    for (int i = 0; i < stack_size; i++) {
        free(dir_stack[i]);
    }
    stack_size = 0;
}

// Push current directory onto stack and change to new directory
int hush_pushd(char **args) {
    char cwd[PATH_MAX];

    // Get current working directory
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("hush: pushd");
        return 1;
    }

    // Check if we have an argument
    if (args[1] == NULL) {
        fprintf(stderr, "hush: pushd: usage: pushd [directory]\n");
        return 1;
    }

    // Check stack overflow
    if (stack_size >= MAX_DIR_STACK) {
        fprintf(stderr, "hush: pushd: directory stack full\n");
        return 1;
    }

    // Try to change to the new directory
    if (chdir(args[1]) != 0) {
        perror("hush: pushd");
        return 1;
    }

    // Save the old directory on the stack
    dir_stack[stack_size++] = strdup(cwd);

    // Display the new directory stack
    return hush_dirs(NULL);
}

// Pop a directory from the stack and change to it
int hush_popd(char **args) {
    // Check if stack is empty
    if (stack_size == 0) {
        fprintf(stderr, "hush: popd: directory stack empty\n");
        return 1;
    }

    // Get the top directory from the stack
    char *top_dir = dir_stack[--stack_size];

    // Change to that directory
    if (chdir(top_dir) != 0) {
        perror("hush: popd");
        // Restore stack position if chdir fails
        stack_size++;
        return 1;
    }

    // Display the new directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    }

    // Free the popped directory
    free(top_dir);

    // Display the new directory stack
    return hush_dirs(NULL);
}

// Display the directory stack
int hush_dirs(char **args) {
    char cwd[PATH_MAX];

    // Get current working directory
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("hush: dirs");
        return 1;
    }

    // Print the current directory first
    printf("%s", cwd);

    // Print the rest of the stack
    for (int i = stack_size - 1; i >= 0; i--) {
        printf(" %s", dir_stack[i]);
    }
    printf("\n");

    return 1;
}
