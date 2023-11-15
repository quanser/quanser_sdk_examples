//////////////////////////////////////////////////////////////////
//
// analog_loopback_example.c - C file
//
// This example performs an analog loopback test at 1kHz.
// It writes a sinewave to analog output channels 0 through 3
// and reads analog input channels 0 through 3 at the same time.
// By wiring analog input channels 0 through 3 to analog output
// channels 0 through 3 respectively, this example tests whether
// the analog values read properly reflect the values written.
//
// Note that the first value read reflects the initial conditions
// and the second value read reflects the voltage written in
// the previous sample. This fact is demonstrated in this example
// by writing 5V to each channel prior to doing the loopback test.
// Since the input and output is synchronized by the
// hil_read_analog_write_analog_buffer function, it is very
// useful for system identification. Looking at the data that
// this example generates, you will see that the very first sample
// read is 5V. Subsequent SAMPLES read will lag the SAMPLES written
// by exactly one sampling instant (1 milliseconds).
//
// This example demonstrates the use of the following functions:
//    hil_open
//    hil_write_analog
//    hil_read_analog_write_analog_buffer
//    hil_close
//
// Copyright (C) 2008 Quanser Inc.
//////////////////////////////////////////////////////////////////
    
#include "analog_loopback_example.h"

int main(int argc, char * argv[])
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
        const t_uint32 input_channels[]    = { 0, 1, 2, 3 };
        const t_uint32 output_channels[]   = { 0, 1, 2, 3 };
        const t_double frequency           = 1000;
        const t_double sine_frequency      = 10; /* frequency of output sinewaves */
        const t_double period              = 1.0 / frequency;

        #define NUM_INPUT_CHANNELS      ARRAY_LENGTH(input_channels)
        #define NUM_OUTPUT_CHANNELS     ARRAY_LENGTH(output_channels)
        #define SAMPLES                 500                             /* 0.5 seconds worth of SAMPLES */

        static t_double initial_voltages[NUM_OUTPUT_CHANNELS];
        static t_double input_voltages[SAMPLES][NUM_INPUT_CHANNELS];
        static t_double output_voltages[SAMPLES][NUM_OUTPUT_CHANNELS];
        
        t_uint channel, index;

        for (channel = 0; channel < NUM_OUTPUT_CHANNELS; channel++)
            initial_voltages[channel] = 5.0;

        for (index = 0; index < SAMPLES; index++)
        {
            t_double time = index * period;
            for (channel = 0; channel < NUM_OUTPUT_CHANNELS; channel++)
                output_voltages[index][channel] = (channel + 7) * sin(2*M_PI*sine_frequency*time);
        }

        result = hil_write_analog(board, output_channels, NUM_OUTPUT_CHANNELS, initial_voltages);
        if (result >= 0)
        {
            result = hil_read_analog_write_analog_buffer(board, SYSTEM_CLOCK_1, frequency, SAMPLES, input_channels, NUM_INPUT_CHANNELS,
                output_channels, NUM_OUTPUT_CHANNELS, &input_voltages[0][0], &output_voltages[0][0]);
            if (result > 0)
            {
                t_uint32 index, channel;
                for (index = 0; index < 7; index++)
                {
                    t_double time = index * period;

                    printf("t=%.3f  ", time);
                    for (channel = 0; channel < NUM_INPUT_CHANNELS; channel++)
                        printf("ADC #%d: %6.3f   ", input_channels[channel], input_voltages[index][channel]);
                    printf("\n");

                    printf("t=%.3f  ", time);
                    for (channel = 0; channel < NUM_OUTPUT_CHANNELS; channel++)
                        printf("DAC #%d: %6.3f   ", output_channels[channel], output_voltages[index][channel]);
                    printf("\n\n");
                }
            }
            else
            {
                msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
                printf("Unable to read-write buffer. %s Error %d.\n", message, -result);
            }
        }            
        else
        {
            msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
            printf("Unable to write initial analog output voltages. %s Error %d.\n", message, -result);
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
