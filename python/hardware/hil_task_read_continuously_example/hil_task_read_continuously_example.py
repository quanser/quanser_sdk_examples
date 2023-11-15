#################################################################
#
# hil_task_read_continuously_example.py - Python file
#
# This example reads from analog channels 0 and 1, and encoder channels 0 and 1,
# every 0.5 seconds. It continues to read until Ctrl-C is pressed.
#
# This example demonstrates the use of the following functions:
#    HIL.open
#    HIL.task_create_reader
#    HIL.task_start
#    HIL.task_read
#    HIL.task_stop
#    HIL.task_delete
#    HIL.close
#
# Copyright (C) 2023 Quanser Inc.
#################################################################
    
from quanser.hardware import HIL, HILError, Clock
import signal
import sys
import array as arr

stop = False

def signal_handler(signum, frame):
    global stop
    stop = True

# Register a Ctrl+C handler
signal.signal(signal.SIGINT, signal_handler)

board_type = "q2_usb"
board_identifier = "0"

try:
    card = HIL()
    card.open(board_type, board_identifier)

    try:
        samples           = HIL.INFINITE # read continuously
        samples_to_read   = 1
        frequency         = 2
        samples_in_buffer = frequency

        analog_channels   = arr.array("i", [0, 1])
        encoder_channels  = arr.array("i", [0, 1])

        num_analog_channels  = len(analog_channels)
        num_encoder_channels = len(encoder_channels)

        voltages = arr.array("d", [0.0] * num_analog_channels)
        counts   = arr.array("i", [0] * num_encoder_channels)

        print("This example reads the first two analog input channels and encoder channels")
        print("two times a second, continuously.")
        
        print("Press CTRL-C to stop reading.\n")
        
        task = card.task_create_reader(samples_in_buffer, analog_channels, num_analog_channels,
            encoder_channels, num_encoder_channels, None, 0, None, 0)

        card.task_start(task, Clock.HARDWARE_CLOCK_0, frequency, samples)

        samples_read = card.task_read(task, samples_to_read, voltages, counts, None, None)
        while samples_read > 0 and stop == False:
            for channel in range(num_analog_channels) :
                print("ADC #%d: %6.3f" % (analog_channels[channel], voltages[channel]), end = "  ")
            for channel in range(num_encoder_channels) :
                print("ENC #%d: %5d" % (encoder_channels[channel], counts[channel]), end = "  ")
            print(flush=True)
            samples_read = card.task_read(task, samples_to_read, voltages, counts, None, None)

        card.task_stop(task)
        card.task_delete(task)

    except HILError as ex:
        print("Unable to run task. %s" % ex.get_error_message())

    finally:
        card.close()

except HILError as ex:
    print("Unable to open board. %s" % ex.get_error_message())

input("Press Enter to continue.")
exit(0)
