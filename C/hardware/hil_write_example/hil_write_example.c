//////////////////////////////////////////////////////////////////
//
// hil_write_example.c - C file
//
// This example writes one sample immediately to two analog output channels
// and four digital output channels.
//
// This example demonstrates the use of the following functions:
//    hil_open
//    hil_set_digital_directions
//    hil_write
//    hil_close
//
// Copyright (C) 2008 Quanser Inc.
//////////////////////////////////////////////////////////////////
    
#include "hil_write_example.h"

int main(int argc, char* argv[])
{
    static const char board_type[]       = "q2_usb";
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
        const t_uint32 analog_channels[]  = { 0, 1 };
        const t_uint32 digital_channels[] = { 0, 1, 2, 3 };

        #define NUM_ANALOG_CHANNELS     ARRAY_LENGTH(analog_channels)
        #define NUM_DIGITAL_CHANNELS    ARRAY_LENGTH(digital_channels)

        t_double  voltages[NUM_ANALOG_CHANNELS];
        t_boolean values[NUM_DIGITAL_CHANNELS];
        t_uint32 channel;

        for (channel = 0; channel < NUM_ANALOG_CHANNELS; channel++)
            voltages[channel] = (channel - 1.5);
        for (channel = 0; channel < NUM_DIGITAL_CHANNELS; channel++)
            values[channel] = channel & 1;

        printf("This example writes constant values to the first four analog and\n");
        printf("digital output channels. The values written are:\n\n");
        for (channel = 0; channel < NUM_ANALOG_CHANNELS; channel++)
            printf("    DAC[%d] = %4.1f V\n", analog_channels[channel], voltages[channel]);
        printf("\n");
        for (channel = 0; channel < NUM_DIGITAL_CHANNELS; channel++)
            printf("    DIG[%d] = %d\n", digital_channels[channel], values[channel]);

        result = hil_set_digital_directions(board, NULL, 0, digital_channels, NUM_DIGITAL_CHANNELS);
        if (result == 0)
        {
            result = hil_write(board, analog_channels, NUM_ANALOG_CHANNELS, NULL, 0, digital_channels, NUM_DIGITAL_CHANNELS,
                NULL, 0, voltages, NULL, values, NULL);
            if (result < 0)
            {
                msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
                printf("Unable to write channels. %s Error %d.\n", message, -result);
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
