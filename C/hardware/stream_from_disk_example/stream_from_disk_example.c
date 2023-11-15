//////////////////////////////////////////////////////////////////
//
// stream_from_disk_example.c - C file
//
// This example demonstrates the double buffering capabilities of the Quanser HIL SDK.
// It writes continuously to analog output channels 0, 1, and 2, at a sampling 
// rate of 1kHz. The data is read from a file as it is output.
//
// A task has created to handle the data generation using hil_task_create_analog_writer. 
// Data collection is started using the hil_task_start function.
// The data is then written to the task's internal buffer in one second
// intervals (1000 samples), using the hil_task_write_analog function.
// 
// Stop the example by pressing Ctrl+C.
//
// This example demonstrates the use of the following functions:
//    hil_open
//    hil_task_create_analog_writer
//    hil_task_start
//    hil_task_write_analog
//    hil_task_flush
//    hil_task_stop
//    hil_task_delete
//    hil_close
//
// Copyright (C) 2008 Quanser Inc.
//////////////////////////////////////////////////////////////////
    
#include "stream_from_disk_example.h"

static int stop = 0;

void signal_handler(int signal)
{
    stop = 1;
}

static t_int 
generate_sample_data_file(char *filename, t_double frequency, t_double duration, t_double sine_frequency,
                          const t_uint channels[], t_uint num_channels)
{
    t_int result = -1;
    FILE *file_handle;
    t_double time;
   
    if (fopen_s(&file_handle, filename, "wt") == 0)
    {
        t_uint channel;
        t_double period = 1.0 / frequency;
        result = 0;

        for (channel = 0; channel < num_channels; channel++)
            fprintf(file_handle, "DAC #%d\t", channel);
        fprintf(file_handle, "\n");

        for (time = 0.0; time < duration; time += period)
        {
            t_uint channel;
            for (channel = 0; channel < num_channels; channel++)
                fprintf(file_handle, "%f\t", (channel + 7.0) * sin(2*M_PI*sine_frequency*time));
            fprintf(file_handle, "\n");
        }
        fclose(file_handle);
    }

    return result;
}

static t_uint 
read_values(FILE *file_handle, t_uint samples_to_write, t_uint num_channels, t_double *voltages)
{
    t_uint samples_read;

    for (samples_read = 0; samples_read < samples_to_write; samples_read++)
    {
        static char line[512];

        if (fgets(line, sizeof(line), file_handle))
        {
            t_uint channel;
            char *position = &line[0];

            for (channel = 0; channel < num_channels && position != NULL; channel++)
                voltages[samples_read*num_channels + channel] = strtod(position, &position);
        }
        else
            break;
    }

    return samples_read;
}

int main(int argc, char* argv[])
{
    static const char board_type[]       = "q8";
    static const char board_identifier[] = "0";
    static char       message[512];
    static const char default_filename[] = "sample_data.txt";

    const t_uint32   samples           = -1; /* write continuously */
    const t_uint32   channels[]        = { 0, 1, 2 };
    const t_double   frequency         = 1000;
    const t_double   sine_frequency    = 100;
    const t_double   duration          = 10.0;  /* run for 10 seconds */

    #define NUM_CHANNELS        ARRAY_LENGTH(channels)
    #define SAMPLES_TO_WRITE    1000    /* one second's worth */

    const t_uint32   samples_in_buffer = (t_uint32)(2*SAMPLES_TO_WRITE); /* double buffer */
    const t_double   period            = 1.0 / frequency;

    static char filename[_MAX_PATH];

    FILE * file_handle;
    qsigaction_t action;
    t_card board;
    t_int  result;

    /* Catch Ctrl+C so application may shut down cleanly */
    action.sa_handler = signal_handler;
    action.sa_flags   = 0;
    qsigemptyset(&action.sa_mask);

    qsigaction(SIGINT, &action, NULL);

    printf("Enter the name of the file to which to write sample data [%s]:\n", default_filename);
    fgets(filename, ARRAY_LENGTH(filename), stdin); /* use fgets to avoid deprecation warnings with gets() on some systems */
    filename[string_length(filename, sizeof(filename)) - 1] = '\0';  /* remove newline */
    if (filename[0] == '\0')
        string_copy(filename, sizeof(filename), default_filename);

    printf("Generating sample data file. Please wait...\n\n");
    if (generate_sample_data_file(filename, frequency, duration, sine_frequency, channels, NUM_CHANNELS) != 0)
    {
        printf("Unable to generate a sample data file\n");
        return -1;
    }

    if (fopen_s(&file_handle, filename, "rt") == 0)
    {
        static char line[512];

        /* Skip header line in the file */
        fgets(line, sizeof(line), file_handle);

        result = hil_open(board_type, board_identifier, &board);
        if (result == 0)
        {
            static t_double voltages[SAMPLES_TO_WRITE][NUM_CHANNELS];

            t_int     samples_written = SAMPLES_TO_WRITE;
            t_int     samples_read;
            t_task    task;

            printf("This example reads data from a sample data file and writes it to\n");
            printf("the first three analog channels as the data is read. The example\n");
            printf("will run for %g seconds or until it is stopped manually.\n", duration);

            printf("Press CTRL-C to stop writing.\n\n");

            result = hil_task_create_analog_writer(board, samples_in_buffer, channels, NUM_CHANNELS, &task);
            if (result == 0)
            {
                samples_read = read_values(file_handle, SAMPLES_TO_WRITE, NUM_CHANNELS, &voltages[0][0]);

                result = hil_task_start(task, SYSTEM_CLOCK_1, frequency, samples);
                if (result == 0)
                {
                    while (samples_read > 0 && samples_written > 0 && stop == 0)
                    {
                        samples_written = hil_task_write_analog(task, samples_read, &voltages[0][0]);
                        samples_read = read_values(file_handle, SAMPLES_TO_WRITE, NUM_CHANNELS, &voltages[0][0]);
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

            hil_close(board);
        }
        else
        {
            msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
            printf("Unable to open board. %s Error %d.\n", message, -result);
        }
    }
    else
        printf("Unable to open data file: \"%s\".\n", filename);


    return 0;
}
