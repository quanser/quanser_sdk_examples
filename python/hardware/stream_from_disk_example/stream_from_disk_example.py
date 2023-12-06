#################################################################
#
# stream_from_disk_example.py - Python file
#
# This example demonstrates the double buffering capabilities of the Quanser HIL SDK.
# It writes continuously to analog output channels 0, 1, and 2, at a sampling 
# rate of 1kHz. The data is read from a file as it is output.
#
# A task has created to handle the data generation using HIL.task_create_analog_writer. 
# Data collection is started using the HIL.task_start function.
# The data is then written to the task's internal buffer in one second
# intervals (1000 samples), using the HIL.task_write_analog function.
# 
# Stop the example by pressing Ctrl+C.
#
# This example demonstrates the use of the following functions:
#    HIL.open
#    HIL.task_create_analog_writer
#    HIL.task_start
#    HIL.task_write_analog
#    HIL.task_flush
#    HIL.task_stop
#    HIL.task_delete
#    HIL.close
#
# Copyright (C) 2023 Quanser Inc.
#################################################################
    
from quanser.hardware import HIL, HILError, Clock
import signal
import math
import re
import array as arr

stop = False

def signal_handler(signum, frame):
    global stop
    stop = True

# Register a Ctrl+C handler
signal.signal(signal.SIGINT, signal_handler)

board_type = "q8_usb"
board_identifier = "0"

def generate_sample_data_file(filename, frequency, duration, sine_frequency, channels):

    num_channels = len(channels)
    file_handle = open(filename, "wt")

    period = 1 / frequency

    for channel in range(num_channels):
        print("  DAC #%d" % channel, end = "\t", file=file_handle)
    print(file=file_handle)

    time = 0.0
    while time < duration:
        for channel in range(num_channels):
            print("%8.3f" % ((channel + 7.0) * math.sin(2 * math.pi * sine_frequency * time)), end="\t", file=file_handle)
        print(file=file_handle)
        time += period

    file_handle.close()

# Returns (float, endpos)
def strtod(s, position):
    m = re.match(r'[+-]?\d*[.]?\d*(?:[eE][+-]?\d+)?', s[position:])
    if m.group(0) == '':
        raise ValueError('Invalid floating-point number: %s' % s[position:])

    return float(m.group(0)), position + m.end()

def read_values(file_handle, samples_to_write, num_channels, voltages):

    for samples_read in range(samples_to_write):
        line = file_handle.readline()

        position = 0
        for channel in range(num_channels):
            voltages[samples_read * num_channels + channel], position = strtod(line, position)

    return samples_read

try:
    card = HIL()
    card.open(board_type, board_identifier)

    try:
        samples           = HIL.INFINITE # read continuously
        samples_to_write  = 1000
        frequency         = 1000
        sine_frequency    = 100
        duration          = 10.0  # Run for 10 seconds
        samples_in_buffer = 2 * samples_to_write # double buffer
        period            = 1 / frequency
        default_filename  = "sample_data.txt"

        channels = arr.array("i", [0, 1, 2])
        num_channels  = len(channels)

        voltages = arr.array("d", [0.0] * samples_to_write * num_channels)

        filename = input("Enter the name of the file to which to write sample data [" + default_filename + "]:")
        if len(filename) == 0:
            filename = default_filename

        generate_sample_data_file(filename, frequency, duration, sine_frequency, channels)

        file_handle = open(filename, "rt")

        # Skip header line in the file
        file_handle.readline()

        time = 0
        samples_written = samples_to_write

        print("This example reads data from a sample data file and writes it to")
        print("the first three analog channels as the data is read. The example")
        print("will run for %g seconds or until it is stopped manually." % duration)

        print("Press CTRL-C to stop writing.\n")
        
        task = card.task_create_analog_writer(samples_in_buffer, channels, num_channels)

        samples_read = read_values(file_handle, samples_to_write, num_channels, voltages)

        card.task_start(task, Clock.HARDWARE_CLOCK_0, frequency, samples)

        while samples_read > 0 and samples_written > 0 and stop == False:
            samples_written = card.task_write_analog(task, samples_read, voltages)
            samples_read = read_values(file_handle, samples_to_write, num_channels, voltages)

        card.task_flush(task)
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
