//////////////////////////////////////////////////////////////////
//
// qube_servo2_usb_control_example.c - C file
//
// This example demonstrates how to do proportional control using the Quanser HIL SDK.
// It is designed to control Quanser's Qube Servo2 USB experiment. The position of the
// motor is read from encoder channel 0. Analog output channel 0 is used to
// drive the motor.
//
// This example runs until Ctrl+C is pressed.
//
// This example demonstrates the use of the following functions:
//    hil_open
//    hil_set_encoder_counts
//    hil_task_create_encoder_reader
//    hil_task_start
//    hil_task_read_encoder
//    hil_write_analog
//    hil_task_stop
//    hil_task_delete
//    hil_close
//
// Copyright (C) 2018 Quanser Inc.
//////////////////////////////////////////////////////////////////
    
#include "qube_servo2_usb_control_example.h"

static int stop = 0;

void signal_handler(int signal)
{
    stop = 1;
}

int main(int argc, char* argv[])
{
    static const char board_type[]       = "qube_servo2_usb";
    static const char board_identifier[] = "0";
    static char       message[512];

    qsigaction_t action;
    t_card board;
    t_int  result;

    /* Catch Ctrl+C so application may shut down cleanly */
    action.sa_handler = signal_handler;
    action.sa_flags   = 0;
    qsigemptyset(&action.sa_mask);

    qsigaction(SIGINT, &action, NULL);

    result = hil_open(board_type, board_identifier, &board);
    if (result == 0)
    {
        const t_uint32 samples              = -1; /* read continuously */
        const t_uint32 analog_channel       = 0;
        const t_uint32 digital_channel      = 0;
        const t_uint32 encoder_channel      = 0;
        const t_double frequency            = 1000;
        const t_double sine_frequency       = 0.5; /* frequency of command signal */
        const t_uint32 samples_in_buffer    = (t_uint32)(0.1 * frequency);
        const t_double period               = 1.0 / frequency;

        t_int32   count;
        t_double  voltage;
        t_boolean enable;
        t_int     samples_read = 0;
        t_task    task;

        printf("This example controls the Quanser Qube Servo2 USB experiment at %g Hz.\n", frequency);
        
        printf("Press CTRL-C to stop the controller.\n\n");
        
        do
        {
            qsched_param_t scheduling_parameters;

            /* Make sure the motor voltage is zero */
            voltage = 0;
            result = hil_write_analog(board, &analog_channel, 1, &voltage);
            if (result < 0)
                break;

            /* Reset the encoder counter to zero */
            count = 0;
            result = hil_set_encoder_counts(board, &encoder_channel, 1, &count);
            if (result < 0)
                break;

            /* Enable the motor */
            enable = 1;
            result = hil_write_digital(board, &digital_channel, 1, &enable);
            if (result < 0)
                break;

            /* Increase our thread priority so we get better sample time performance */
            scheduling_parameters.sched_priority = qsched_get_priority_max(QSCHED_FIFO);
            qthread_setschedparam(qthread_self(), QSCHED_FIFO, &scheduling_parameters);

            /* Create a task to read the encoder. The task will be used to time the control loop */
            result = hil_task_create_encoder_reader(board, samples_in_buffer, &encoder_channel, 1, &task);
            if (result < 0)
                break;

            /* Start the task, which will cause it to start reading the encoder at the given rate */
            result = hil_task_start(task, HARDWARE_CLOCK_0, frequency, samples);
            if (result == 0)
            {
                const t_double gain = 0.3;

                t_double position;
                t_double error;
                t_double command = 0;
                t_double time = 0;

                /* The proportional control loop */
                samples_read = hil_task_read_encoder(task, 1, &count);
                while (samples_read > 0 && stop == 0)
                {
                    position = count * 360 / 2048;     /* convert counts to degrees */
                    error    = command - position;     /* compute error in position */
                    voltage  = gain * error;           /* apply proportional control */
                        
                    hil_write_analog(board, &analog_channel, 1, &voltage);
                    
                    /* Compute command signal for next sampling instant */
                    time += period;
                    command = 45 * sin(2*M_PI*sine_frequency*time);
                    samples_read = hil_task_read_encoder(task, 1, &count);
                }

                hil_task_stop(task);
            }
            else
            {
                msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
                printf("Unable to start task. %s Error %d.\n", message, -result);
            }

            /* Turn off motor */
            voltage = 0;
            hil_write_analog(board, &analog_channel, 1, &voltage);

            enable = 0;
            hil_write_digital(board, &digital_channel, 1, &enable);

            if (samples_read < 0)
            {
                msg_get_error_message(NULL, samples_read, message, ARRAY_LENGTH(message));
                printf("Unable to read encoder channel. %s Error %d.\n", message, -samples_read);
            }
            else
            {
                printf("\nController has been stopped. Press Enter to continue.\n");
                getchar(); /* absorb Ctrl-C */
            }

            hil_task_delete(task);
        } while (false);

        if (result < 0)
        {
            msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
            printf("%s Error %d.\n", message, -result);
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
