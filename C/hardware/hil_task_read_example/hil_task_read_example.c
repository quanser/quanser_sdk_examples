//////////////////////////////////////////////////////////////////
//
// hil_task_read_example.c - C file
//
// This example reads from analog channels 0 and 1, and encoder channels 0 and 1,
// every 0.5 seconds. It stops reading when 20 samples have been read, or Ctrl-C
// is pressed.
//
// This example demonstrates the use of the following functions:
//    hil_open
//    hil_task_create_reader
//    hil_task_start
//    hil_task_read
//    hil_task_stop
//    hil_task_delete
//    hil_close
//
// Copyright (C) 2008 Quanser Inc.
//////////////////////////////////////////////////////////////////
    
#include "hil_task_read_example.h"

static int stop = 0;

void signal_handler(int signal)
{
    stop = 1;
}

int main(int argc, char* argv[])
{
    static const char board_type[]       = "q2_usb";
    static const char board_identifier[] = "0";
    static char       message[512];

    qsigaction_t action;
    t_card board;
    t_int  result;

    /* Catch Ctrl+C so application may be shut down cleanly */
    action.sa_handler = signal_handler;
    action.sa_flags   = 0;
    qsigemptyset(&action.sa_mask);

    qsigaction(SIGINT, &action, NULL);

    result = hil_open(board_type, board_identifier, &board);
    if (result == 0)
    {
        const t_uint32   samples_to_read      = 1;
        const t_uint32   analog_channels[]    = { 0, 1 };
        const t_uint32   encoder_channels[]   = { 0, 1 };
        const t_double   frequency            = 2;
        const t_uint32   samples_in_buffer    = (t_uint32) frequency;

        #define NUM_ANALOG_CHANNELS     ARRAY_LENGTH(analog_channels)
        #define NUM_ENCODER_CHANNELS    ARRAY_LENGTH(encoder_channels)
        #define SAMPLES                 20  /* read 20 samples */

        t_int32  counts[NUM_ENCODER_CHANNELS];
        t_double voltages[NUM_ANALOG_CHANNELS];
        t_int    samples_read;
        t_task   task;

        printf("This example reads the first two analog input channels and encoder channels\n");
        printf("two times a second, for %g seconds.\n\n", SAMPLES / frequency);
        
        result = hil_task_create_reader(board, samples_in_buffer, analog_channels, NUM_ANALOG_CHANNELS,
            encoder_channels, NUM_ENCODER_CHANNELS, NULL, 0, NULL, 0, &task);
        if (result == 0)
        {
            result = hil_task_start(task, HARDWARE_CLOCK_0, frequency, SAMPLES);
            if (result == 0)
            {
                samples_read = hil_task_read(task, samples_to_read, voltages, counts, NULL, NULL);
                while (samples_read > 0 && stop == 0)
                {
                    t_uint channel;
                    for (channel = 0; channel < NUM_ANALOG_CHANNELS; channel++)
                        printf("ADC #%d: %6.3f  ", analog_channels[channel], voltages[channel]);
                    for (channel = 0; channel < NUM_ENCODER_CHANNELS; channel++)
                        printf("ENC #%d: %5d    ", encoder_channels[channel], counts[channel]);
                    printf("\n");
                    samples_read = hil_task_read(task, samples_to_read, voltages, counts, NULL, NULL);
                }

                hil_task_stop(task);

                if (samples_read < 0)
                {
                    msg_get_error_message(NULL, samples_read, message, ARRAY_LENGTH(message));
                    printf("Unable to read channels. %s Error %d.\n", message, -samples_read);
                }
                else
                {
                    printf("\nRead operation has been stopped. Press Enter to continue.\n");
                    getchar(); /* absorb Ctrl+C */
                }
            }
            else
            {
                msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
                printf("Unable to start task. %s Error %d.\n", message, -result);
            }

            hil_task_delete(task);
        }
        else
        {
            msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
            printf("Unable to create task. %s Error %d.\n", message, -result);
        }
            
        hil_close(board);
    }
    else
    {
        msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
        printf("Unable to open board. %s Error %d.\n", message, -result);
    }

    return 0;
}
