//////////////////////////////////////////////////////////////////
//
// hil_write_analog_performance.c - C file
//
// This example determines how quickly an analog channel may be written.
// It gives a very rough estimate of the maximum sampling frequency possible
// using the immediate I/O functions writing one sample at a time.
//
// This performance demonstrates the use of the following functions:
//    hil_open
//    hil_write_analog
//    hil_close
//
// Copyright (C) 2008 Quanser Inc.
//////////////////////////////////////////////////////////////////
    
#include "hil_write_analog_performance.h"

int main(int argc, char* argv[])
{
    static const char board_type[]       = "q8";
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
        const t_uint32 channels[]   = { 0 };
        const t_uint   iterations   = 100000;

        #define NUM_CHANNELS    ARRAY_LENGTH(channels)

        qsched_param_t scheduling_parameters;
        t_double voltages[NUM_CHANNELS];
        t_uint   index;

        t_uint32 channel;
        t_timeout start_time, stop_time, interval;

        for (channel = 0; channel < NUM_CHANNELS; channel++)
            voltages[channel] = (channel - 1.5);

        printf("Running %d iterations of hil_write_analog.\n", iterations);

        scheduling_parameters.sched_priority = qsched_get_priority_max(QSCHED_FIFO);
        qthread_setschedparam(qthread_self(), QSCHED_FIFO, &scheduling_parameters);

        timeout_get_high_resolution_time(&start_time);

        for (index = iterations; index > 0 && result >= 0; --index)
            result = hil_write_analog(board, channels, NUM_CHANNELS, voltages);

        timeout_get_high_resolution_time(&stop_time);
        timeout_subtract(&interval, &stop_time, &start_time);

        if (result >= 0)
        {
            double time = interval.seconds + interval.nanoseconds * 1e-9;
            printf("%d iterations took %f seconds\n(%.0f Hz or %.1f usecs per call)\n", iterations,
                time, iterations / time, time / iterations * 1e6);
        }
        else
        {
            msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
            printf("Unable to write channels. %s Error %d.\n", message, -result);
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
