//////////////////////////////////////////////////////////////////
//
// hil_write_analog_buffer_example.c - C file
//
// This example writes 10,000 samples to analog output channels 0 through 3.
// The samples consist of 100 Hz sine waves with different amplitudes.
//
// This example demonstrates the use of the following functions:
//    hil_open
//    hil_write_analog_buffer
//    hil_close
//
// Copyright (C) 2008 Quanser Inc.
//////////////////////////////////////////////////////////////////
    
#include "hil_write_analog_buffer_example.h"

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

        static t_double voltages[SAMPLES][NUM_CHANNELS];

        t_int    samples_written;
        t_double period = 1.0 / frequency;
        t_uint   channel, index;

        printf("This example writes sine waves to the first 4 analog output channels\n");
        printf("for %g seconds. The sinewave frequency is %g Hz. The amplitudes are:\n",
            SAMPLES / frequency, sine_frequency);
        for (channel = 0; channel < NUM_CHANNELS; channel++)
        {
            printf("    DAC[%d] = %4.1f Vpp (%.3g Vrms)\n", channels[channel], channel + 7.0, 
                (channel + 7.0) * M_SQRT1_2);
        }
           
        for (index = 0; index < SAMPLES; index++)
        {
            t_double time = index * period;
            for (channel = 0; channel < NUM_CHANNELS; channel++)
                voltages[index][channel] = (channel + 7) * sin(2*M_PI*sine_frequency*time);
        }

        samples_written = hil_write_analog_buffer(board, SYSTEM_CLOCK_1, frequency, SAMPLES, channels, NUM_CHANNELS, &voltages[0][0]);
        if (samples_written < 0)
        {
            msg_get_error_message(NULL, samples_written, message, ARRAY_LENGTH(message));
            printf("Unable to write buffer. %s Error %d.\n", message, -samples_written);
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
