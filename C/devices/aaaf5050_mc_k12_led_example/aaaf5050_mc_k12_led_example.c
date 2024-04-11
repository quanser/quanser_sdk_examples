#include "pch.h"

#define MAX_LEDS    33

typedef struct tag_aaaf5050_mc_k12* t_aaaf5050_mc_k12;

EXTERN t_error
aaaf5050_mc_k12_open(const char* uri, t_uint max_leds, t_aaaf5050_mc_k12* led_handle);

EXTERN t_int
aaaf5050_mc_k12_write(t_aaaf5050_mc_k12 led_handle, const t_led_color* colors, t_uint num_leds);

EXTERN t_error
aaaf5050_mc_k12_close(t_aaaf5050_mc_k12 led_handle);

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
    t_aaaf5050_mc_k12 led_strip;
    qsigaction_t action;
    t_led_color colors[MAX_LEDS];
    t_error result;

    /* Install a Ctrl+C handler */
    action.sa_handler = signal_handler;
    action.sa_flags = 0;
    qsigemptyset(&action.sa_mask);

    qsigaction(SIGINT, &action, NULL);

    printf("Press Ctrl+C to exit gracefully\n");
    fflush(stdout);

    /* Open the Kingsbright AAAF5050-MC-K12 LED led_strip. */
    result = aaaf5050_mc_k12_open("spi://localhost:1?memsize=417,word='8',baud='3333333',lsb='off',frame='1'", MAX_LEDS, &led_strip);
    if (result >= 0)
    {
        t_timeout pause = { 0, 100000000, false }; /* 0.1 seconds */
        t_int offset = 0;
        t_uint i;

        for (i = 0; i < MAX_LEDS; i++)
        {
            colors[i].red   = (t_uint8)(    i * (255 / MAX_LEDS));
            colors[i].green = (t_uint8)(2 * i * (255 / MAX_LEDS));
            colors[i].blue  = (t_uint8)(4 * i * (255 / MAX_LEDS));
        }

        while (!stop)
        {
            t_led_color temp_color;

            /* Set the colors of the LEDs in the strip */
            result = aaaf5050_mc_k12_write(led_strip, colors, ARRAY_LENGTH(colors));
            if (result < 0)
                break;

            qtimer_sleep(&pause);

            /* Rotate the colors */
            memory_copy(&temp_color, sizeof(temp_color), &colors[0]);
            memory_move(&colors[0], (MAX_LEDS - 1) * sizeof(colors[0]), &colors[1]);
            memory_copy(&colors[MAX_LEDS - 1], sizeof(colors[0]), &temp_color);
        }

        /* Turn off LEDs */
        for (i = 0; i < MAX_LEDS; i++)
        {
            colors[i].red   = 0;
            colors[i].green = 0;
            colors[i].blue  = 0;
        }

        aaaf5050_mc_k12_write(led_strip, colors, ARRAY_LENGTH(colors));

        /* Close the led_strip */
        aaaf5050_mc_k12_close(led_strip);
    }

    if (result < 0)
    {
        char message[256];
        msg_get_error_messageA(NULL, result, message, ARRAY_LENGTH(message));
        fprintf(stderr, "ERROR: Unable to write to LED strip. %s (result=%d)\n", message, result);
    }

    return result;
}
