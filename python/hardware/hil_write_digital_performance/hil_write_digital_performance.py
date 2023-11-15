#################################################################
#
# hil_write_digital_performance.py - Python file
#
# This example determines how quickly an digital channel may be written.
# It gives a very rough estimate of the maximum sampling frequency possible
# using the immediate I/O functions writing one sample at a time.
#
# This performance demonstrates the use of the following functions:
#    HIL.open
#    HIL.write_digital
#    HIL.close
#
# Copyright (C) 2023 Quanser Inc.
#################################################################
    
from quanser.common import Timeout
from quanser.hardware import HIL, HILError, Clock
import array as arr

board_type = "q2_usb"
board_identifier = "0"

try:
    card = HIL()
    card.open(board_type, board_identifier)

    try:
        channels = arr.array("i", [0])
        num_channels = len(channels)

        values = arr.array("d", [0] * num_channels)

        iterations = 100000

        for channel in range(num_channels):
            values[channel] = (channel & 1)

        print("Running %d iterations of HIL.write_digital." % iterations)

        start_time = Timeout.get_high_resolution_time()

        for index in range(iterations):
            card.write_digital(channels, num_channels, values)

        interval = Timeout.get_high_resolution_time() - start_time

        time = interval.seconds + interval.nanoseconds * 1e-9
        print("%d iterations took %f seconds\n(%.0f Hz or %.1f usecs per call)" % (iterations, time, iterations / time, time / iterations * 1e6))

    except HILError as ex:
        print("Unable to write channels. %s" % ex.get_error_message())

    finally:
        card.close()

except HILError as ex:
    print("Unable to open board. %s" % ex.get_error_message())


input("Press Enter to exit.")
exit(0)
