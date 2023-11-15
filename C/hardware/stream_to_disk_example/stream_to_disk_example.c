//////////////////////////////////////////////////////////////////
//
// stream_to_disk_example.c - C file
//
// This example demonstrates the double buffering capabilities of the C API.
// It reads continuously from analog input channels 0 and 1 and encoder
// channels 0 and 1, at a sampling rate of 1kHz. The data is written to
// a file as it is collected.
//
// Data collection is started using the hil_task_start function.
// The data is then read from the C API's internal buffer in one second
// intervals (1000 samples), using the hil_task_read function.
//
// Stop the example by pressing Ctrl-C. 
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
    
#include "stream_to_disk_example.h"

static int stop = 0;

void signal_handler(int signal)
{
    stop = 1;
}

int main(int argc, char * argv[])
{
    static const char board_type[]       = "q2_usb";
    static const char board_identifier[] = "0";
    static char       message[512];

    qsigaction_t action;
    t_card board;
    t_int  result;

    /* Catch Ctrl+C to shut down application cleanly */
    action.sa_handler = signal_handler;
    action.sa_flags   = 0;
    qsigemptyset(&action.sa_mask);

    qsigaction(SIGINT, &action, NULL);

    result = hil_open(board_type, board_identifier, &board);
    if (result == 0)
    {
        const t_uint32  samples              = -1; /* read continuously */
        const t_uint32  analog_channels[]    = { 0, 1 };
        const t_uint32  encoder_channels[]   = { 0, 1 };
        const t_double  frequency            = 1000;

        #define NUM_ANALOG_CHANNELS     ARRAY_LENGTH(analog_channels)
        #define NUM_ENCODER_CHANNELS    ARRAY_LENGTH(encoder_channels)
        #define SAMPLES_TO_READ         1000    /* one second's worth */

        const t_uint32  samples_in_buffer    = (t_uint32)(2 * SAMPLES_TO_READ); /* double buffer */
        const t_double  period               = 1.0 / frequency;

        static t_int32  counts[SAMPLES_TO_READ][NUM_ENCODER_CHANNELS];
        static t_double voltages[SAMPLES_TO_READ][NUM_ANALOG_CHANNELS];
        static const char default_filename[] = "data.txt";

        t_int  samples_read;
        t_task task;
        char filename[_MAX_PATH];
        FILE * file_handle;

        printf("This example reads the first two analog input channels and encoder channels\n");
        printf("at %g Hz, continuously, writing the data to a file.\n", frequency);
        
        printf("Press CTRL-C to stop reading.\n\n");
        printf("Enter the name of the file to which to write the data [%s]:\n", default_filename);
        fgets(filename, ARRAY_LENGTH(filename), stdin); /* use fgets to avoid deprecation warnings that occur on some systems */
        filename[string_length(filename, sizeof(filename)) - 1] = '\0';
        if (filename[0] == '\0')
            string_copy(filename, sizeof(filename), default_filename);
        
        if (stdfile_open(filename, "wt", &file_handle) == 0)
        {
            t_double time = 0;

            printf("\nWriting to the file. Each dot represents %d data points.\n", SAMPLES_TO_READ);

            result = hil_task_create_reader(board, samples_in_buffer, analog_channels, NUM_ANALOG_CHANNELS,
                encoder_channels, NUM_ENCODER_CHANNELS, NULL, 0, NULL, 0, &task);
            if (result == 0)
            {
                result = hil_task_start(task, HARDWARE_CLOCK_0, frequency, samples);
                if (result == 0)
                {
                    samples_read = hil_task_read(task, SAMPLES_TO_READ, &voltages[0][0], &counts[0][0], NULL, NULL);
                    while (samples_read > 0 && stop == 0)
                    {
                        t_uint channel;
                        t_int index;

                        printf(".");
                        fflush(stdout);

                        for (index = 0; index < samples_read; index++)
                        {
                            fprintf(file_handle, "t: %8.4f  ", time);
                            time += period;

                            for (channel = 0; channel < NUM_ANALOG_CHANNELS; channel++)
                                fprintf(file_handle, "ADC #%d: %5.3f    ", analog_channels[channel], voltages[index][channel]);
                            for (channel = 0; channel < NUM_ENCODER_CHANNELS; channel++)
                                fprintf(file_handle, "ENC #%d: %5d    ", encoder_channels[channel], counts[index][channel]);
                            fprintf(file_handle, "\n");
                        }

                        samples_read = hil_task_read(task, SAMPLES_TO_READ, &voltages[0][0], &counts[0][0], NULL, NULL);
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

            stdfile_close(file_handle);
        }
        else
            printf("Unable to open file \"%s\".\n", filename);

        hil_close(board);
    }
    else
    {
        msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
        printf("Unable to open board. %s Error %d.\n", message, -result);
    }

    return 0;
}
