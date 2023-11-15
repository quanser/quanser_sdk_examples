#################################################################
#
# hil_write_analog_example.py - Python file
#
# This example writes one sample immediately to four analog output channels.
#
# This example demonstrates the use of the following functions:
#    HIL.open
#    HIL.write_analog
#    HIL.close
#
# Copyright (C) 2023 Quanser Inc.
#################################################################

from quanser.hardware import HIL, HILError, Clock
import math
import array as arr

board_type = "q2_usb"
board_identifier = "0"

try:
    card = HIL()
    card.open(board_type, board_identifier)

    try:
        channels = arr.array("i", [0, 1])
        num_channels = len(channels)

        voltages = arr.array("d", [0.0] * num_channels)

        for channel in range(num_channels):
            voltages[channel] = (channel - 1.5)

        print("This example writes constant voltages to the first %d analog output" % num_channels)
        print("channels. The voltages written are:")
        for channel in range(num_channels):
            print("    DAC[%d] = %4.1f V" % (channels[channel], voltages[channel]))

        card.write_analog(channels, num_channels, voltages)

    except HILError as ex:
        print("Unable to write channels. %s" % ex.get_error_message())

    finally:
        card.close()

except HILError as ex:
    print("Unable to open board. %s" % ex.get_error_message())

input("Press Enter to continue.")
exit(0)
