//////////////////////////////////////////////////////////////////
//
// hil_read_analog_example.c - C file
//
// This example reads one sample immediately from four analog input channels.
//
// This example demonstrates the use of the following functions:
//    hil_open
//    hil_read_analog
//    hil_close
//
// Copyright (C) 2008 Quanser Inc.
//////////////////////////////////////////////////////////////////
    
#include "hil_read_analog_example.h"

int main(int argc, char* argv[])
{
    static const char  board_type[]       = "q2_usb";
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
        const t_uint32 channels[] = { 0, 1 };

        #define NUM_CHANNELS    ARRAY_LENGTH(channels)

        t_double voltages[NUM_CHANNELS];

        result = hil_read_analog(board, channels, NUM_CHANNELS, &voltages[0]);
        if (result >= 0)
        {
            t_uint32 channel;
            for (channel = 0; channel < NUM_CHANNELS; channel++)
                printf("ADC #%d: %7.4f   ", channels[channel], voltages[channel]);
            printf("\n");
        }
        else
        {
            msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
            printf("Unable to read channels. %s Error %d.\n", message, -result);
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
