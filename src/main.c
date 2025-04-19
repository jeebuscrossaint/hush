#include "main.h"
#include "loop.h"
#include "signals.h"
#include "history.h"
#include "readline.h"
#include "alias.h"
#include "dir_stack.h"
#include "control.h"
#include "jobs.h"
#include "variables.h"

int execute_script(const char *filename, int argc, char **argv) {
    FILE *script = fopen(filename, "r");
    if (!script) {
        perror("hush");
        return 1;
    }

    // Set script arguments ($0, $1, $2, ...)
    // $0 is the script name itself
    char **script_args = malloc((argc + 1) * sizeof(char *));
    if (script_args) {
        script_args[0] = strdup(filename);
        for (int i = 1; i <= argc; i++) {
            script_args[i] = strdup(argv[i-1]);
        }
        set_script_args(argc + 1, script_args);

        // Free temp array (contents are now owned by set_script_args)
        for (int i = 0; i <= argc; i++) {
            free(script_args[i]);
        }
        free(script_args);
    }

    int result = process_script_control_flow(script);
    fclose(script);
    return result;
}

// Update main function
int main(int argc, char **argv) {
    // Set up our signal handlers
    setup_signal_handlers();

    // Initialize job control
    init_job_control();

    // Initialize variables
    init_shell_variables();

    // Initialize readline
    hush_readline_init();

    // Load command history
    hush_load_history();

    // Initialize aliases
    init_aliases();

    // Initialize directory stack
    init_dir_stack();

    if (argc > 1) {
        // Execute script with remaining arguments
        return execute_script(argv[1], argc - 2, &argv[2]);
    }

    // Start the shell loop
    hush_loop();

    // Save command history before exiting
    hush_save_history();

    // Cleanup readline
    hush_readline_cleanup();

    // Clean up directory stack
    free_dir_stack();

    // Free history memory
    for (int i = 0; i < hush_history_count; i++) {
        free(hush_history_list[i]);
    }

    rl_deprep_terminal();

    return EXIT_SUCCESS;
}
