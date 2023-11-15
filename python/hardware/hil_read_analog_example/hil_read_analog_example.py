#################################################################
#
# hil_read_analog_example.py - Python file
#
# This example reads one sample immediately from two analog input channels.
#
# This example demonstrates the use of the following functions:
#    HIL.open
#    HIL.read_analog
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

        card.read_analog(channels, num_channels, voltages)
        for channel in range(num_channels):
            print("ADC #%d: %7.4f" % (channels[channel], voltages[channel]), end="   ")
        print()

    except HILError as ex:
        print("Unable to read channels. %s" % ex.get_error_message())

    finally:
        card.close()

except HILError as ex:
    print("Unable to open board. %s" % ex.get_error_message())

input("Press Enter to continue.")
exit(0)
