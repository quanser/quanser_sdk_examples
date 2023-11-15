#################################################################
#
# hil_task_write_example.py - Python file
#
# This example writes from analog channels 0 and 1, and encoder channels 0 and 1,
# at a sampling rate of 1 kHz. It stops writing when 10000 samples have been written,
# or Ctrl-C is pressed.
#
# This example demonstrates the use of the following functions:
#    HIL.open
#    HIL.task_create_writer
#    HIL.task_start
#    HIL.task_write
#    HIL.task_flush
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

board_type = "q8"
board_identifier = "0"

try:
    card = HIL()
    card.open(board_type, board_identifier)

    try:
        samples           = 10000 # 10 seconds worth
        samples_to_write  = 1
        frequency         = 1000
        sine_frequency    = 100
        samples_in_buffer = frequency
        period            = 1 / frequency

        analog_channels   = arr.array("i", [0, 1])
        digital_channels  = arr.array("i", [0, 1])

        num_analog_channels  = len(analog_channels)
        num_digital_channels = len(digital_channels)

        voltages = arr.array("d", [0.0] * num_analog_channels)
        values   = arr.array("b", [1] * num_digital_channels)

        print("This example writes square waves to the first two digital output channels");
        print("and sine waves to the first two analog output channels for %g seconds." % (samples / frequency))

        print("The sine wave frequency is %g Hz and the sinewave amplitudes are:" % sine_frequency)
        for channel in range(num_analog_channels):
            print("    DAC[%d] = %4.1f Vpp (%.3g Vrms)" % (analog_channels[channel], channel + 7.0, (channel + 7.0) * math.sqrt(0.5)))

        print("\nThe square wave frequencies are:")
        for channel in range(num_digital_channels):
            print("    DIG[%d] = %4.1f Hz" % (digital_channels[channel], frequency / (channel + 2)))
        
        card.set_digital_directions(None, 0, digital_channels, num_digital_channels)

        task = card.task_create_writer(samples_in_buffer, analog_channels, num_analog_channels,
            None, 0, digital_channels, num_digital_channels, None, 0)

        card.task_start(task, Clock.HARDWARE_CLOCK_0, frequency, samples)

        index = 0
        samples_written = card.task_write(task, samples_to_write, voltages, None, values, None)
        while samples_written > 0 and stop == False:
            time = index * period
            for channel in range(num_analog_channels) :
                voltages[channel] = (channel + 7) * math.sin(2 * math.pi * sine_frequency * time)
            for channel in range(num_digital_channels) :
                values[channel] = (index % (channel + 2) >= (channel + 2) / 2)

            samples_written = card.task_write(task, samples_to_write, voltages, None, values, None)
            index += 1

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
