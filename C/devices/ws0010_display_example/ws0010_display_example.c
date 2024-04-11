#include "pch.h"

/*
* Use Ctrl+C to stop the program gracefully.
*/

static int stop = 0;

static void
signal_handler(int signal)
{
    stop = 1;
}

int main(int argc, char* argv[])
{
    t_ws0010 display;
    qsigaction_t action;
    t_error result;

    /* Install a Ctrl+C handler */
    action.sa_handler = signal_handler;
    action.sa_flags = 0;
    qsigemptyset(&action.sa_mask);

    qsigaction(SIGINT, &action, NULL);

    printf("Press Ctrl+C to exit gracefully\n");
    fflush(stdout);

    /* Open the WS0010 OLED display in text mode. On QBot Platform use the URI: "lcd://localhost:1". */
    result = ws0010_open("spi://localhost:0?baud=2e6,word=10,polarity=1,phase=1,frame=56", false, &display);
    if (result >= 0)
    {
        t_timeout pause = { 0, 100000000, false }; /* 0.1 seconds */
        t_int offset = 0;
        t_int column = 15;

        static const char message[] = "Hello world \020 "; /* use '\020' to print a custom character defined by ws0010_set_character */
        const char next_message[] = "\fCiao for now!\nSee you later!"; /* use '\f' to clear display first */

        static const t_uint8 pattern[8] =
        {
            0x00,  /* 00000 */
            0x0A,  /* 01010 */
            0x0A,  /* 01010 */
            0x00,  /* 00000 */
            0x00,  /* 00000 */
            0x11,  /* 10001 */
            0x0E,  /* 01110 */
            0x00   /* 00000 */
        };

        result = ws0010_set_character(display, '\020', pattern);
        if (result >= 0)
        {
            while (!stop)
            {
                /* Print the text at the given column and from the given offset in the string */
                result = ws0010_print(display, 0, column, &message[offset], ARRAY_LENGTH(message));
                if (result < 0)
                    break;

                qtimer_sleep(&pause);

                /* Scroll the text to the left and wrap when it disappears */
                if (column == 0)
                {
                    offset++;
                    if (offset >= ARRAY_LENGTH(message))
                    {
                        offset = 0;
                        column = 15;
                    }
                }
                else
                    --column;
            }
        }

        ws0010_print(display, 0, 0, next_message, ARRAY_LENGTH(next_message));

        /* Close the display */
        ws0010_close(display);
    }

    if (result < 0)
    {
        char message[256];
        msg_get_error_messageA(NULL, result, message, ARRAY_LENGTH(message));
        fprintf(stderr, "ERROR: Unable to write to display. %s (result=%d)\n", message, result);
    }

    return result;
}
