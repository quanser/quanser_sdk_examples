#################################################################
#
# hil_write_example.py - Python file
#
# This example writes one sample immediately to two analog output channels
# and four digital output channels.
#
# This example demonstrates the use of the following functions:
#    HIL.open
#    HIL.set_digital_directions
#    HIL.write
#    HIL.close
#
# Copyright (C) 2023 Quanser Inc.
#################################################################

from quanser.hardware import HIL, HILError, Clock
import array as arr

board_type = "q2_usb"
board_identifier = "0"

try:
    card = HIL()
    card.open(board_type, board_identifier)

    try:
        analog_channels  = arr.array("i", [0, 1])
        digital_channels = arr.array("i", [0, 1, 2, 3])

        num_analog_channels  = len(analog_channels)
        num_digital_channels = len(digital_channels)

        voltages = arr.array("d", [0.0] * num_analog_channels)
        values   = arr.array("b", [0] * num_digital_channels)

        for channel in range(num_analog_channels):
            voltages[channel] = (channel - 1.5)
        for channel in range(num_digital_channels):
            values[channel] = channel & 1

        print("This example writes constant values to the first four analog and");
        print("digital output channels. The values written are:\n");
        for channel in range(num_analog_channels):
            print("    DAC[%d] = %4.1f V" % (analog_channels[channel], voltages[channel]))
        print()
        for channel in range(num_digital_channels):
            print("    DIG[%d] = %d" % (digital_channels[channel], values[channel]))
        print()

        card.set_digital_directions(None, 0, digital_channels, num_digital_channels)

        card.write(analog_channels, num_analog_channels, 
                   None, 0,
                   digital_channels, num_digital_channels,
                   None, 0,
                   voltages, 
                   None, 
                   values, 
                   None)

    except HILError as ex:
        print("Unable to write channels. %s" % ex.get_error_message())

    finally:
        card.close()

except HILError as ex:
    print("Unable to open board. %s" % ex.get_error_message())

input("Press Enter to continue.")
exit(0)
