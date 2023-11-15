#################################################################
#
# stream_to_disk_example.py - Python file
#
# This example demonstrates the double buffering capabilities of the Python API.
# It reads continuously from analog input channels 0 and 1 and encoder
# channels 0 and 1, at a sampling rate of 1 kHz. The data is written to
# a file as it is collected.
#
# Data collection is started using the HIL.task_start function.
# The data is then read from the Python API's internal buffer in one second
# intervals (1000 samples), using the HIL.task_read function.
#
# Stop the example by pressing Ctrl-C. 
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
        samples_to_read   = 1000
        frequency         = 1000
        samples_in_buffer = 2 * samples_to_read # double buffer
        period            = 1 / frequency
        default_filename  = "data.txt"

        analog_channels   = arr.array("i", [0])
        encoder_channels  = arr.array("i", [0, 1])

        num_analog_channels  = len(analog_channels)
        num_encoder_channels = len(encoder_channels)

        voltages = arr.array("d", [0.0] * samples_to_read * num_analog_channels)
        counts   = arr.array("i", [0] * samples_to_read * num_encoder_channels)

        print("This example reads the first two analog input channels and encoder channels")
        print("at %g Hz, continuously, writing the data to a file." % frequency)
        
        print("Press CTRL-C to stop reading.\n");
        filename = input("Enter the name of the file to which to write the data [" + default_filename + "]:")
        if len(filename) == 0:
            filename = default_filename

        file_handle = open(filename, "wt")

        time = 0

        print("\nWriting to the file. Each dot represents %d data points." % samples_to_read)
        
        task = card.task_create_reader(samples_in_buffer, analog_channels, num_analog_channels,
            encoder_channels, num_encoder_channels, None, 0, None, 0)

        card.task_start(task, Clock.HARDWARE_CLOCK_0, frequency, samples)

        samples_read = card.task_read(task, samples_to_read, voltages, counts, None, None)
        while samples_read > 0 and stop == False:
            print(".", end="", flush=True)

            for index in range(samples_read):
                print("t: %8.4f" % time, end="  ", file=file_handle)
                time += period

                for channel in range(num_analog_channels):
                    print("ADC #%d: %5.3f" % (analog_channels[channel], voltages[index * num_analog_channels + channel]), end="  ", file=file_handle)
                for channel in range(num_encoder_channels):
                    print("ENC #%d: %5d" % (encoder_channels[channel], counts[index * num_encoder_channels + channel]), end="  ", file=file_handle)
                print(flush=True, file=file_handle)

            samples_read = card.task_read(task, samples_to_read, voltages, counts, None, None)

        card.task_stop(task)
        card.task_delete(task)

    except HILError as ex:
        print("\nUnable to run task. %s" % ex.get_error_message())

    finally:
        card.close()

except HILError as ex:
    print("Unable to open board. %s" % ex.get_error_message())

input("\nPress Enter to continue.")
exit(0)
