#include "main.h"
#include "loop.h"
#include "signals.h"
#include "history.h"
#include "readline.h"
#include "alias.h"

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

    // Start the shell loop
    hush_loop();

    // Save command history before exiting
    hush_save_history();

    // Cleanup readline before deprep terminal
    hush_readline_cleanup();

    // Cleanup terminal
    rl_deprep_terminal();

    // Free history memory
    for (int i = 0; i < hush_history_count; i++) {
        free(hush_history_list[i]);
    }

    return EXIT_SUCCESS;
}
