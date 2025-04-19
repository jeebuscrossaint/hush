#include "main.h"
#include "loop.h"
#include "signals.h"
#include "history.h"
#include "readline.h"
#include "alias.h"
#include "dir_stack.h"
#include "control.h"

int execute_script(const char *filename) {
    FILE *script = fopen(filename, "r");
    if (!script) {
        perror("hush");
        return 1;
    }

    int result = process_script_control_flow(script);
    fclose(script);
    return result;
}

int main(int argc, char **argv)
{
    // Set up our signal handlers
    setup_signal_handlers();

    // Initialize readline
    hush_readline_init();

    // Load command history
    hush_load_history();

    // Initialize aliases
    init_aliases();

    // Initialize directory stack
    init_dir_stack();

    if (argc > 1) {
        return execute_script(argv[1]);
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
