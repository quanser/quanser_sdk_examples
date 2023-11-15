#################################################################
#
# hil_read_example.py - Python file
#
# This example reads one sample immediately from two analog input channels,
# two encoder input channels and four digital input channels.
#
# This example demonstrates the use of the following functions:
#    HIL.open
#    HIL.read
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
        analog_channels  = arr.array("i", [0, 1])
        encoder_channels = arr.array("i", [0, 1])
        digital_channels = arr.array("i", [0, 1, 2, 3])

        num_analog_channels  = len(analog_channels)
        num_encoder_channels = len(encoder_channels)
        num_digital_channels = len(digital_channels)

        voltages = arr.array("d", [0.0] * num_analog_channels)
        counts   = arr.array("i", [0] * num_encoder_channels)
        values   = arr.array("b", [0] * num_digital_channels)

        card.read(analog_channels, num_analog_channels, 
                  encoder_channels, num_encoder_channels,
                  digital_channels, num_digital_channels,
                  None, 0,
                  voltages, 
                  counts, 
                  values, 
                  None)

        for channel in range(num_analog_channels) :
            print("ADC #%d: %7.4f" % (analog_channels[channel], voltages[channel]), end = "   ")
        print()
        for channel in range(num_encoder_channels) :
            print("ENC #%d: %7d" % (encoder_channels[channel], counts[channel]), end = "   ")
        print()
        for channel in range(num_digital_channels) :
            print("DIG #%d: %7d" % (digital_channels[channel], values[channel]), end = "   ")
        print()

    except HILError as ex:
        print("Unable to read channels. %s" % ex.get_error_message())

    finally:
        card.close()

except HILError as ex:
    print("Unable to open board. %s" % ex.get_error_message())

input("Press Enter to continue.")
exit(0)
