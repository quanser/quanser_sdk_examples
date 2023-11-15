#################################################################
#
# hil_write_digital_example.py - Python file
#
# This example writes one sample immediately to four digital output channels.
#
# This example demonstrates the use of the following functions:
#    HIL.open
#    HIL.set_digital_directions
#    HIL.write_digital
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
        channels = arr.array("i", [0, 1, 2, 3])
        num_channels = len(channels)

        values = arr.array("b", [0] * num_channels)

        for channel in range(num_channels):
            values[channel] = channel & 1

        print("This example writes constant values to the first %d digital output" % num_channels)
        print("channels. The values written are:")
        for channel in range(num_channels):
            print("    DIG[%d] = %d" % (channels[channel], values[channel]))

        card.set_digital_directions(None, 0, channels, num_channels)
        card.write_digital(channels, num_channels, values)

    except HILError as ex:
        print("Unable to write channels. %s" % ex.get_error_message())

    finally:
        card.close()

except HILError as ex:
    print("Unable to open board. %s" % ex.get_error_message())

input("Press Enter to continue.")
exit(0)
