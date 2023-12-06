//////////////////////////////////////////////////////////////////
//
// hil_read_analog_buffer_example.c - C file
//
// This example reads 20 SAMPLES from analog input channels 0 through 3,
// at a 1kHz sampling rate.
//
// This example demonstrates the use of the following functions:
//    hil_open
//    hil_read_analog_buffer
//    hil_close
//
// Copyright (C) 2008 Quanser Inc.
//////////////////////////////////////////////////////////////////
    
#include "hil_read_analog_buffer_example.h"

int main(int argc, char* argv[])
{
    static const char  board_type[]       = "q8_usb";
    static const char  board_identifier[] = "0";
    static char        message[512];

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
        const t_uint32 channels[]   = { 0, 1, 2, 3 };
        const t_double frequency    = 1000;

        #define NUM_CHANNELS          ARRAY_LENGTH(channels)
        #define SAMPLES               20

        static t_double voltages[SAMPLES][NUM_CHANNELS];
        t_int           samples_read;

        samples_read = hil_read_analog_buffer(board, SYSTEM_CLOCK_1, frequency, SAMPLES, channels, NUM_CHANNELS, &voltages[0][0]);
        if (samples_read > 0)
        {
            t_uint32 index, channel;
            for (index = 0; index < SAMPLES; index++)
            {
                for (channel = 0; channel < NUM_CHANNELS; channel++)
                    printf("ADC #%d: %7.4f   ", channels[channel], voltages[index][channel]);
                printf("\n");
            }
        }
        else
        {
            msg_get_error_message(NULL, samples_read, message, ARRAY_LENGTH(message));
            printf("Unable to read buffer. %s Error %d.\n", message, -samples_read);
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
