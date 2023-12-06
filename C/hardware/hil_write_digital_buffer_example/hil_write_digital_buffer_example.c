//////////////////////////////////////////////////////////////////
//
// hil_write_digital_buffer_example.c - C file
//
// This example writes 10,000 samples to digital I/O channels 0 through 3.
// The samples consist of square waves of differing frequencies.
//
// This example demonstrates the use of the following functions:
//    hil_open
//    hil_set_digital_directions
//    hil_write_digital_buffer
//    hil_close
//
// Copyright (C) 2008 Quanser Inc.
//////////////////////////////////////////////////////////////////
    
#include "hil_write_digital_buffer_example.h"

int main(int argc, char* argv[])
{
    static const char board_type[]       = "q8_usb";
    static const char board_identifier[] = "0";
    static char       message[512];

    qsigaction_t action;
    t_card board;
    t_int  result;

    /* Prevent Ctrl+C from stopping the application so hil_close gets called */
    action.sa_handler = SIG_IGN;
    action.sa_flags   = 0;
    qsigemptyset(&action.sa_mask);

    qsigaction(SIGINT, &action, NULL);

    result = hil_open(board_type, board_identifier, &board);
    if (result == 0)
    {
        const t_uint32 channels[]     = { 0, 1, 2, 3 };
        const t_double frequency      = 1000;
        const t_double sine_frequency = 100;

        #define NUM_CHANNELS        ARRAY_LENGTH(channels)
        #define SAMPLES             10000  /* writes 10,000 samples */

        static t_boolean values[SAMPLES][NUM_CHANNELS];

        t_int     samples_written;
        t_uint    channel, index;

        printf("This example writes square waves to the first 4 digital output channels\n");
        printf("for %g seconds. The square wave frequencies are:\n", SAMPLES / frequency);
        for (channel = 0; channel < NUM_CHANNELS; channel++)
            printf("    DIG[%d] = %4.1f Hz\n", channels[channel], frequency / (channel + 2));
           
        /* Compute square waves with the desired frequencies */
        for (index = 0; index < SAMPLES; index++)
        {
            for (channel = 0; channel < NUM_CHANNELS; channel++)
                values[index][channel] = ((index % (channel + 2) >= (channel + 2) / 2));
        }

        result = hil_set_digital_directions(board, NULL, 0, channels, NUM_CHANNELS);
        if (result == 0)
        {
            /* Output the computed waveforms */
            samples_written = hil_write_digital_buffer(board, SYSTEM_CLOCK_1, frequency, SAMPLES, channels, NUM_CHANNELS, &values[0][0]);
            if (samples_written < 0)
            {
                msg_get_error_message(NULL, samples_written, message, ARRAY_LENGTH(message));
                printf("Unable to write buffer. %s Error %d.\n", message, -samples_written);
            }
        }
        else
        {
            msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
            printf("Unable to set digital directions. %s Error %d.\n", message, -result);
        }

        hil_close(board);
    }
    else
    {
        msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
        printf("Unable to open board. %s Error %d.\n", message, -result);
    }

    printf("\nPress Enter to continue.\n");
    getchar();

    return 0;
}
