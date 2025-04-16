#include "readline.h"
#include <errno.h>
#include "signals.h"
#include <unistd.h>
#include <termios.h>
#include <ctype.h>

char *hush_read_line(void)
{
    // Allocate a buffer
    char *buffer = malloc(HUSH_RL_BUFSIZE);
    if (!buffer) {
        perror("hush: allocation error");
        exit(EXIT_FAILURE);
    }

    size_t position = 0;
    size_t buffer_size = HUSH_RL_BUFSIZE;
    int c;

    while (1) {
        // Read a character
        c = getchar();

        // If we get EOF or error
        if (c == EOF) {
            if (position == 0) {
                // Empty line with EOF
                free(buffer);
                printf("\n");
                exit(EXIT_SUCCESS);
            } else {
                // EOF in the middle of a line, just treat it as a newline
                buffer[position] = '\0';
                return buffer;
            }
        }

        // If we got a newline, terminate the line and return
        if (c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            // Otherwise add the character to the buffer
            buffer[position] = c;
        }
        position++;

        // If we've exceeded the buffer, reallocate
        if (position >= buffer_size) {
            buffer_size += HUSH_RL_BUFSIZE;
            buffer = realloc(buffer, buffer_size);
            if (!buffer) {
                perror("hush: allocation error");
                exit(EXIT_FAILURE);
            }
        }
    }
}
