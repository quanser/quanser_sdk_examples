#################################################################
#
# qube_servo2_usb_control_example.py - Python file
#
# This example demonstrates how to do proportional control using the Quanser HIL SDK.
# It is designed to control Quanser's Qube Servo2 USB experiment. The position of the
# motor is read from encoder channel 0. Analog output channel 0 is used to
# drive the motor.
#
# This example runs until Ctrl+C is pressed.
#
# This example demonstrates the use of the following functions:
#    HIL.open
#    HIL.set_encoder_counts
#    HIL.task_create_encoder_reader
#    HIL.task_start
#    HIL.task_read_encoder
#    HIL.write_analog
#    HIL.task_stop
#    HIL.task_delete
#    HIL.close
#
# Copyright (C) 2023 Quanser Inc.
#################################################################
    
from quanser.hardware import HIL, HILError, Clock
import math
import signal
import sys
import array as arr

stop = False

def signal_handler(signum, frame):
    global stop
    stop = True

# Register a Ctrl+C handler
signal.signal(signal.SIGINT, signal_handler)

board_type = "qube_servo2_usb"
board_identifier = "0"

try:
    card = HIL()
    card.open(board_type, board_identifier)

    try:
        samples           = HIL.INFINITE # write continuously
        samples_to_write  = 1
        frequency         = 1000
        sine_frequency    = 0.5 # frequency of command signal
        samples_in_buffer = math.ceil(0.1 * frequency)
        period            = 1 / frequency

        analog_channels   = arr.array("i", [0])
        digital_channels  = arr.array("i", [0])
        encoder_channels  = arr.array("i", [0])

        num_analog_channels  = len(analog_channels)
        num_encoder_channels = len(encoder_channels)
        num_digital_channels = len(digital_channels)

        voltages = arr.array("d", [0.0] * num_analog_channels)
        counts   = arr.array("i", [0] * num_encoder_channels)
        states   = arr.array("b", [1] * num_digital_channels)

        print("This example controls the Quanser Qube Servo2 USB experiment at %g Hz." % frequency)
        
        print("Press CTRL-C to stop the controller.\n")

        # Make sure the motor voltage is zero
        card.write_analog(analog_channels, num_analog_channels, voltages)

        # Reset the encoder counts to zero
        card.set_encoder_counts(encoder_channels, num_encoder_channels, counts)

        # Enable the motor
        card.write_digital(digital_channels, num_digital_channels, states)

        # Create a task to read the encoder. The task will be used to time the control loop
        task = card.task_create_encoder_reader(samples_in_buffer, encoder_channels, num_encoder_channels)
          
        # Start the task, which will cause it to start reading the encoder at the given rate */
        card.task_start(task, Clock.HARDWARE_CLOCK_0, frequency, samples)

        # Begin control
        gain = 0.3
        command = 0
        time = 0

        samples_read = card.task_read_encoder(task, 1, counts)
        while samples_read > 0 and stop == False:

            position    = counts[0] * 360 / 2048 # convert counts to degrees
            error       = command - position     # compute error in position
            voltages[0] = gain * error           # apply proportional control
                        
            card.write_analog(analog_channels, num_analog_channels, voltages)
                    
            # Compute command signal for next sampling instant
            time += period
            command = 45 * math.sin(2 * math.pi * sine_frequency * time)
            samples_read = card.task_read_encoder(task, 1, counts)

        card.task_stop(task)
        card.task_delete(task)

        # Turn off motor
        voltages[0] = 0.0
        card.write_analog(analog_channels, num_analog_channels, voltages)

        # Disable motor
        states[0] = 0
        card.write_digital(digital_channels, num_digital_channels, states)

    except HILError as ex:
        print("Unable to run task. %s" % ex.get_error_message())

    finally:
        card.close()

except HILError as ex:
    print("Unable to open board. %s" % ex.get_error_message())

input("Press Enter to continue.")
exit(0)
