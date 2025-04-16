#include "main.h"
#include "loop.h"
#include "signals.h"
#include "history.h"
#include "readline.h"

int main(int argc, char **argv)
{
    // Set up our signal handlers
    setup_signal_handlers();

    // Load command history
    load_history();

    // Make sure terminal gets reset properly if shell exits
    atexit(disable_raw_mode);

    // Start the shell loop
    hush_loop();

    // Save command history before exiting
    save_history();

    // Free history memory
    for (int i = 0; i < history_count; i++) {
        free(history_list[i]);
    }

    return EXIT_SUCCESS;
}
