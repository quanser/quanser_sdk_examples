//////////////////////////////////////////////////////////////////
//
// hil_write_buffer_example.c - C file
//
// This example writes 10,000 samples to analog output channels 0 and 1,
// and to digital I/O channels 0 and 1, at a sampling rate 1kHz. The
// samples consist of two 100 Hz sinewaves of differing amplitudes to
// the analog channels and two square waves of differing frequencies 
// to the digital channels.
//
// This example demonstrates the use of the following functions:
//    hil_open
//    hil_write_buffer
//    hil_close
//
// Copyright (C) 2008 Quanser Inc.
//////////////////////////////////////////////////////////////////
    
#include "hil_write_buffer_example.h"

int main(int argc, char* argv[])
{
    static const char board_type[]       = "q8";
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
        const t_uint32 analog_channels[]    = { 0, 1 };
        const t_uint32 digital_channels[]   = { 0, 1 };
        const t_double frequency            = 1000;
        const t_double sine_frequency       = 100;

        #define NUM_ANALOG_CHANNELS     ARRAY_LENGTH(analog_channels)
        #define NUM_DIGITAL_CHANNELS    ARRAY_LENGTH(digital_channels)
        #define SAMPLES                 10000  /* writes 10,000 samples */

        static t_double  voltages[SAMPLES][NUM_ANALOG_CHANNELS];
        static t_boolean values[SAMPLES][NUM_DIGITAL_CHANNELS];

        t_int     samples_written;
        t_double  period = 1.0 / frequency;
        t_uint    channel, index;

        printf("This example writes square waves to the first two digital output channels\n");
        printf("and sine waves to the first two analog output channels for %g seconds.\n", SAMPLES / frequency);
        printf("\nThe sine wave frequency is %g Hz and the sinewave amplitudes are:\n", sine_frequency);
        for (channel = 0; channel < NUM_ANALOG_CHANNELS; channel++)
        {
            printf("    DAC[%d] = %4.1f Vpp (%.3g Vrms)\n", analog_channels[channel], channel + 7.0, 
                (channel + 7.0) * M_SQRT1_2);
        }
        printf("\nThe square wave frequencies are:\n");
        for (channel = 0; channel < NUM_DIGITAL_CHANNELS; channel++)
            printf("    DIG[%d] = %4.1f Hz\n",  digital_channels[channel], frequency / (channel + 2));

        for (index = 0; index < SAMPLES; index++)
        {
            t_double time = index * period;
            for (channel = 0; channel < NUM_ANALOG_CHANNELS; channel++)
                voltages[index][channel] = (channel + 7) * sin(2*M_PI*sine_frequency*time);
        }

        /* Compute square waves with the desired frequencies */
        for (index = 0; index < SAMPLES; index++)
        {
            for (channel = 0; channel < NUM_DIGITAL_CHANNELS; channel++)
                values[index][channel] = ((index % (channel + 2) >= (channel + 2) / 2));
        }

        result = hil_set_digital_directions(board, NULL, 0, digital_channels, NUM_DIGITAL_CHANNELS);
        if (result == 0)
        {
            samples_written = hil_write_buffer(board, SYSTEM_CLOCK_1, frequency, SAMPLES, analog_channels, NUM_ANALOG_CHANNELS,
                NULL, 0, digital_channels, NUM_DIGITAL_CHANNELS, NULL, 0, &voltages[0][0], NULL, &values[0][0], NULL);
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
