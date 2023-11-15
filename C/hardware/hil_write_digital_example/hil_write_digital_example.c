//////////////////////////////////////////////////////////////////
//
// hil_write_digital_example.c - C file
//
// This example writes one sample immediately to four digital output channels.
//
// This example demonstrates the use of the following functions:
//    hil_open
//    hil_set_digital_directions
//    hil_write_digital
//    hil_close
//
// Copyright (C) 2008 Quanser Inc.
//////////////////////////////////////////////////////////////////
    
#include "hil_write_digital_example.h"

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
        const t_uint32 channels[]   = { 0, 1, 2, 3 };

        #define NUM_CHANNELS    ARRAY_LENGTH(channels)

        t_boolean values[NUM_CHANNELS];
        t_uint32 channel;

        for (channel = 0; channel < NUM_CHANNELS; channel++)
            values[channel] = channel & 1;

        printf("This example writes constant values to the first four digital output\n");
        printf("channels. The values written are:\n");
        for (channel = 0; channel < NUM_CHANNELS; channel++)
            printf("    DIG[%d] = %d\n", channels[channel], values[channel]);

        result = hil_set_digital_directions(board, NULL, 0, channels, NUM_CHANNELS);
        if (result == 0)
        {
            result = hil_write_digital(board, channels, NUM_CHANNELS, values);
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
