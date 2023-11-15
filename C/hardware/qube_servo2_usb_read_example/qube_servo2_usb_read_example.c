//////////////////////////////////////////////////////////////////
//
// qube_servo2_usb_read_example.c - C file
//
// This example reads one sample immediately from all the input
// channels of the Qube Servo2 USB.
//
// This example demonstrates the use of the following functions:
//    hil_open
//    hil_read
//    hil_close
//
// Copyright (C) 2018 Quanser Inc.
//////////////////////////////////////////////////////////////////
    
#include "qube_servo2_usb_read_example.h"

int main(int argc, char* argv[])
{
    static const char board_type[]       = "qube_servo2_usb";
    static const char board_identifier[] = "0";
    static char       message[512];

    qsigaction_t action;
    t_card board;
    t_int  result;

    /* Prevent Ctrl+C from stopping the application so hil_close gets called */
    action.sa_handler = SIG_IGN;
    action.sa_flags   = 0;
    qsigemptyset(&action.sa_mask);

    result = hil_open(board_type, board_identifier, &board);
    if (result == 0)
    {
        const t_uint32 analog_channels[]   = { 0 };       /* motor current sense (A) */
        const t_uint32 encoder_channels[]  = { 0, 1 };    /* motor encoder, pendulum encoder (counts) */
        const t_uint32 digital_channels[]  = { 0, 1, 2 }; /* amplifier fault, stall detected, stall error */
        const t_uint32 other_channels[]    = { 14000 };   /* tachometer (counts/sec) */

        #define NUM_ANALOG_CHANNELS     ARRAY_LENGTH(analog_channels)
        #define NUM_ENCODER_CHANNELS    ARRAY_LENGTH(encoder_channels)
        #define NUM_DIGITAL_CHANNELS    ARRAY_LENGTH(digital_channels)
        #define NUM_OTHER_CHANNELS      ARRAY_LENGTH(other_channels)
        #define SAMPLES                 20

        t_double  voltages[NUM_ANALOG_CHANNELS];
        t_int32   counts[NUM_ENCODER_CHANNELS];
        t_boolean states[NUM_DIGITAL_CHANNELS];
        t_double  values[NUM_OTHER_CHANNELS];
        qsched_param_t scheduling_parameters;

        /* Increase our thread priority so we respond to the USB packets fast enough */
        scheduling_parameters.sched_priority = qsched_get_priority_max(QSCHED_FIFO);
        qthread_setschedparam(qthread_self(), QSCHED_FIFO, &scheduling_parameters);

        result = hil_read(board, analog_channels, NUM_ANALOG_CHANNELS, encoder_channels, NUM_ENCODER_CHANNELS,
            digital_channels, NUM_DIGITAL_CHANNELS, other_channels, NUM_OTHER_CHANNELS,
            &voltages[0], &counts[0], &states[0], &values[0]);
        if (result >= 0)
        {
            t_uint32 channel;
            for (channel = 0; channel < NUM_ANALOG_CHANNELS; channel++)
                printf("ADC #%d:     %7.4f   ", analog_channels[channel], voltages[channel]);
            printf("\n");
            for (channel = 0; channel < NUM_ENCODER_CHANNELS; channel++)
                printf("ENC #%d:     %7d   ", encoder_channels[channel], counts[channel]);
            printf("\n");
            for (channel = 0; channel < NUM_DIGITAL_CHANNELS; channel++)
                printf("DIG #%d:     %7d   ", digital_channels[channel], states[channel]);
            printf("\n");
            for (channel = 0; channel < NUM_OTHER_CHANNELS; channel++)
                printf("OTH #%d: %7.0f   ", other_channels[channel], values[channel]);
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
