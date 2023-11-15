//////////////////////////////////////////////////////////////////
//
// hil_task_write_example.c - C file
//
// This example writes to analog channels 0 and 1, and digital channels 0 and 1,
// at a sampling rate of 1 kHz. It stops writing when 20 samples have been written,
// or Ctrl-C is pressed.
//
// This example demonstrates the use of the following functions:
//    hil_open
//    hil_task_create_writer
//    hil_task_start
//    hil_task_write
//    hil_task_flush
//    hil_task_stop
//    hil_task_delete
//    hil_close
//
// Copyright (C) 2008 Quanser Inc.
//////////////////////////////////////////////////////////////////
    
#include "hil_task_write_example.h"

static int stop = 0;

void signal_handler(int signal)
{
    stop = 1;
}

int main(int argc, char* argv[])
{
    static const char board_type[]       = "q8";
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
        const t_uint32   samples_to_write     = 1;
        const t_uint32   analog_channels[]    = { 0, 1 };
        const t_uint32   digital_channels[]   = { 0, 1 };
        const t_double   frequency            = 1000;
        const t_double   sine_frequency       = 100;
        const t_uint32   samples_in_buffer    = (t_uint32)frequency;
        const t_double   period               = 1.0 / frequency;

        #define NUM_ANALOG_CHANNELS     ARRAY_LENGTH(analog_channels)
        #define NUM_DIGITAL_CHANNELS    ARRAY_LENGTH(digital_channels)
        #define SAMPLES                 10000  /* 10 seconds worth */

        t_double  voltages[NUM_ANALOG_CHANNELS];
        t_boolean values[NUM_DIGITAL_CHANNELS];
        t_int     samples_written;
        t_uint    channel;
        t_task    task;

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

        for (channel = 0; channel < NUM_ANALOG_CHANNELS; channel++)
            voltages[channel] = 0;

        for (channel = 0; channel < NUM_DIGITAL_CHANNELS; channel++)
            values[channel] = 1;

        result = hil_set_digital_directions(board, NULL, 0, digital_channels, NUM_DIGITAL_CHANNELS);
        if (result == 0)
        {
            result = hil_task_create_writer(board, samples_in_buffer, analog_channels, NUM_ANALOG_CHANNELS,
                NULL, 0, digital_channels, NUM_DIGITAL_CHANNELS, NULL, 0, &task);
            if (result == 0)
            {
                result = hil_task_start(task, SYSTEM_CLOCK_1, frequency, SAMPLES);
                if (result == 0)
                {
                    t_uint index = 0;
                    samples_written = hil_task_write(task, samples_to_write, voltages, NULL, values, NULL);
                    while (samples_written > 0 && stop == 0)
                    {
                        t_double time = index * period;
                        for (channel = 0; channel < NUM_ANALOG_CHANNELS; channel++)
                            voltages[channel] = (channel + 7) * sin(2*M_PI*sine_frequency*time);

                        for (channel = 0; channel < NUM_DIGITAL_CHANNELS; channel++)
                            values[channel] = ((index % (channel + 2) >= (channel + 2) / 2));

                        samples_written = hil_task_write(task, samples_to_write, voltages, NULL, values, NULL);
                        index++;
                    }

                    hil_task_flush(task);
                    hil_task_stop(task);

                    if (samples_written < 0)
                    {
                        msg_get_error_message(NULL, samples_written, message, ARRAY_LENGTH(message));
                        printf("Unable to write channels. %s Error %d.\n", message, -samples_written);
                    }
                    else
                    {
                        printf("\nWrite operation has been stopped. Press Enter to continue.\n");
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

    return 0;
}
