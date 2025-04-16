#include "readline.h"
#include "history.h"
#include <errno.h>
#include "signals.h"
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <sys/ioctl.h>

// Terminal attributes
static struct termios orig_termios;
static int terminal_initialized = 0;

// Functions for terminal control
void enable_raw_mode() {
    if (isatty(STDIN_FILENO)) {
        // Get the current terminal attributes
        if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
            perror("tcgetattr");
            exit(EXIT_FAILURE);
        }
        terminal_initialized = 1;

        // Modify the attributes for raw mode
        struct termios raw = orig_termios;
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        raw.c_cflag |= (CS8);
        raw.c_oflag &= ~(OPOST);

        // Set timeout for read()
        raw.c_cc[VMIN] = 0;  // Return as soon as any data is available
        raw.c_cc[VTIME] = 1; // Wait up to 0.1 seconds

        // Set the modified attributes
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
            perror("tcsetattr");
            exit(EXIT_FAILURE);
        }
    }
}

void disable_raw_mode() {
    if (terminal_initialized && isatty(STDIN_FILENO)) {
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
            perror("tcsetattr");
        }
    }
}

// Function to get a single keypress
int read_key() {
    int nread;
    char c;

    // Read a single character
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) {
            perror("read");
            exit(EXIT_FAILURE);
        }
    }

    // Handle escape sequences
    if (c == '\x1b') {
        // It's an escape sequence, read more characters
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        // Handle arrow keys
        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }

        return '\x1b';
    }

    return c;
}

// Get the cursor position
int get_cursor_position(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;

    // Query the terminal for cursor position
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    // Read the response
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    // Parse the position
    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
}

// Get the terminal size
int get_window_size(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        // If ioctl fails, try to get the size by moving the cursor
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return get_cursor_position(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

// Refresh the line display
void refresh_line(char *buf, int pos, int len, int prompt_len) {
    char *ab = malloc(len + prompt_len + 20);
    if (!ab) return;

    int abpos = 0;

    // Move cursor to beginning of line
    ab[abpos++] = '\r';

    // Write the prompt
    memcpy(ab + abpos, "$ ", prompt_len);
    abpos += prompt_len;

    // Write the buffer
    memcpy(ab + abpos, buf, len);
    abpos += len;

    // Erase to end of line
    ab[abpos++] = '\x1b';
    ab[abpos++] = '[';
    ab[abpos++] = 'K';

    // Move cursor to correct position
    ab[abpos++] = '\r';

    // Calculate columns for cursor positioning
    int cols = prompt_len + pos;
    if (cols > 0) {
        ab[abpos++] = '\x1b';
        ab[abpos++] = '[';

        char colbuf[20];
        int colbuflen = snprintf(colbuf, sizeof(colbuf), "%d", cols);
        memcpy(ab + abpos, colbuf, colbuflen);
        abpos += colbuflen;

        ab[abpos++] = 'C';
    }

    // Write the buffer to stdout
    if (write(STDOUT_FILENO, ab, abpos) == -1) { /* Ignore error */ }

    free(ab);
}

// Main readline function with history navigation
char *hush_read_line(void) {
    // Enable raw mode for terminal input
    enable_raw_mode();

    // Allocate buffer for the line
    char *buffer = malloc(HUSH_RL_BUFSIZE);
    if (!buffer) {
        perror("hush: allocation error");
        exit(EXIT_FAILURE);
    }

    size_t bufsize = HUSH_RL_BUFSIZE;
    size_t position = 0;
    size_t length = 0;
    int history_index = history_count;
    char *current_line = NULL;

    // Variables for terminal size
    int terminal_rows, terminal_cols;
    get_window_size(&terminal_rows, &terminal_cols);

    buffer[0] = '\0';

    // Main input loop
    while (1) {
        int c = read_key();

        switch (c) {
            case CTRL_KEY('c'):
                // Ctrl-C, clear line and show new prompt
                buffer[0] = '\0';
                length = position = 0;
                refresh_line(buffer, position, length, 2);
                continue;

            case '\r':
            case '\n':
                // Enter key - finish editing
                write(STDOUT_FILENO, "\r\n", 2);
                disable_raw_mode();

                if (length == 0) {
                    buffer[0] = '\0';
                } else {
                    buffer[length] = '\0';
                }

                if (current_line) free(current_line);
                return buffer;

            case BACKSPACE:
            case CTRL_KEY('h'):
            case DELETE_KEY:
                // Backspace or delete key
                if (position > 0) {
                    // Remove character at left of cursor
                    memmove(&buffer[position-1], &buffer[position], length - position + 1);
                    position--;
                    length--;
                    refresh_line(buffer, position, length, 2);
                }
                break;

            case ARROW_LEFT:
                // Move cursor left
                if (position > 0) {
                    position--;
                    refresh_line(buffer, position, length, 2);
                }
                break;

            case ARROW_RIGHT:
                // Move cursor right
                if (position < length) {
                    position++;
                    refresh_line(buffer, position, length, 2);
                }
                break;

            case ARROW_UP:
                // Navigate history up
                if (history_index > 0) {
                    // Save the current line if it's the first time pressing up
                    if (history_index == history_count && length > 0) {
                        if (current_line) free(current_line);
                        current_line = strdup(buffer);
                    }

                    history_index--;
                    strcpy(buffer, history_list[history_index]);
                    length = position = strlen(buffer);
                    refresh_line(buffer, position, length, 2);
                }
                break;

            case ARROW_DOWN:
                // Navigate history down
                if (history_index < history_count) {
                    history_index++;

                    if (history_index == history_count) {
                        // Restore the current line
                        if (current_line) {
                            strcpy(buffer, current_line);
                            length = position = strlen(buffer);
                        } else {
                            buffer[0] = '\0';
                            length = position = 0;
                        }
                    } else {
                        strcpy(buffer, history_list[history_index]);
                        length = position = strlen(buffer);
                    }

                    refresh_line(buffer, position, length, 2);
                }
                break;

            case HOME_KEY:
                // Move cursor to beginning of line
                position = 0;
                refresh_line(buffer, position, length, 2);
                break;

            case END_KEY:
                // Move cursor to end of line
                position = length;
                refresh_line(buffer, position, length, 2);
                break;

            case CTRL_KEY('l'):
                // Clear screen (Ctrl+L)
                write(STDOUT_FILENO, "\x1b[2J", 4);
                write(STDOUT_FILENO, "\x1b[H", 3);
                refresh_line(buffer, position, length, 2);
                break;

            default:
                // Regular character
                if (c >= 32 && c < 127) {
                    // Only printable ASCII characters
                    if (length >= bufsize - 1) {
                        bufsize *= 2;
                        buffer = realloc(buffer, bufsize);
                        if (!buffer) {
                            perror("hush: allocation error");
                            exit(EXIT_FAILURE);
                        }
                    }

                    // Make space for new character
                    memmove(&buffer[position+1], &buffer[position], length - position + 1);

                    // Insert character
                    buffer[position] = c;
                    position++;
                    length++;

                    refresh_line(buffer, position, length, 2);
                }
                break;
        }
    }
}
