#include "main.h"
#include "loop.h"
#include "signals.h"

int main(int argc, char **argv)
{
    // Set up our signal handlers
    setup_signal_handlers();

    // Start the shell loop
    hush_loop();

    return EXIT_SUCCESS;
}
