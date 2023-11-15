//////////////////////////////////////////////////////////////////
//
// qube_servo2_usb_write_example.c - C file
//
// This example writes one sample immediately to all the outputs
// of the Qube Servo2 USB.
//
// This example demonstrates the use of the following functions:
//    hil_open
//    hil_set_digital_directions
//    hil_write
//    hil_close
//
// Copyright (C) 2018 Quanser Inc.
//////////////////////////////////////////////////////////////////
    
#include "qube_servo2_usb_write_example.h"

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

    qsigaction(SIGINT, &action, NULL);

    result = hil_open(board_type, board_identifier, &board);
    if (result == 0)
    {
        const t_uint32 analog_channels[]   = { 0 };                   /* motor voltage */
        const t_uint32 digital_channels[]  = { 0 };                   /* motor enable */
        const t_uint32 other_channels[]    = { 11000, 11001, 11002 }; /* LED red, green and blue */

        enum analog_channel_indices  { MOTOR_VOLTAGE };
        enum digital_channel_indices { MOTOR_ENABLE };
        enum other_channel_indices   { LED_RED, LED_GREEN, LED_BLUE };

        #define NUM_ANALOG_CHANNELS  ARRAY_LENGTH(analog_channels)
        #define NUM_DIGITAL_CHANNELS ARRAY_LENGTH(digital_channels)
        #define NUM_OTHER_CHANNELS   ARRAY_LENGTH(other_channels)

        t_double  voltages[NUM_ANALOG_CHANNELS];
        t_boolean states[NUM_DIGITAL_CHANNELS];
        t_double  values[NUM_OTHER_CHANNELS];
        qsched_param_t scheduling_parameters;
        t_uint32  channel;

        voltages[MOTOR_VOLTAGE] = 1.0;   /* 1V to motor */
        states[MOTOR_ENABLE]    = 1;     /* enable motor */
        values[LED_RED]         = 1;     /* red component */
        values[LED_GREEN]       = 0.5;   /* green component */
        values[LED_BLUE]        = 0.2;   /* blue component */
       
        printf("This example moves the motor for one second and changes the LED\n");
        printf("colour. The values written are::\n\n");
        for (channel = 0; channel < NUM_ANALOG_CHANNELS; channel++)
            printf("    DAC[%d] = %4.1f V\n", analog_channels[channel], voltages[channel]);
        printf("\n");
        for (channel = 0; channel < NUM_DIGITAL_CHANNELS; channel++)
            printf("    DIG[%d] = %d\n", digital_channels[channel], states[channel]);
        printf("\n");
        for (channel = 0; channel < NUM_OTHER_CHANNELS; channel++)
            printf("    OTH[%d] = %1.2f\n", other_channels[channel], values[channel]);
        printf("\n");

        /* Increase our thread priority so we respond to the USB packets fast enough */
        scheduling_parameters.sched_priority = qsched_get_priority_max(QSCHED_FIFO);
        qthread_setschedparam(qthread_self(), QSCHED_FIFO, &scheduling_parameters);

        result = hil_set_digital_directions(board, NULL, 0, digital_channels, NUM_DIGITAL_CHANNELS);
        if (result == 0)
        {
            result = hil_write(board, analog_channels, NUM_ANALOG_CHANNELS, NULL, 0,
                               digital_channels, NUM_DIGITAL_CHANNELS, other_channels, NUM_OTHER_CHANNELS,
                               voltages, NULL, states, values);
            if (result < 0)
            {
                msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
                printf("Unable to write channels. %s Error %d.\n", message, -result);
            }

            /* Sleep for one second */
            sleep(ONE_SECOND);

            /* Stop the motor and restore LED to red */

            voltages[MOTOR_VOLTAGE] = 0; /* set motor voltage to zero */
            states[MOTOR_ENABLE]    = 0; /* turn motor off */
            values[LED_RED]         = 1; /* pure red */
            values[LED_GREEN]       = 0;
            values[LED_BLUE]        = 0;

            hil_write(board, analog_channels, NUM_ANALOG_CHANNELS, NULL, 0,
                      digital_channels, NUM_DIGITAL_CHANNELS, other_channels, NUM_OTHER_CHANNELS,
                      voltages, NULL, states, values);
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
