#################################################################
#
# qube_servo2_usb_read_example.py - Python file
#
# This example reads one sample immediately from all the input
# channels of the Qube Servo2 USB.
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

board_type = "qube_servo2_usb"
board_identifier = "0"

try:
    card = HIL()
    card.open(board_type, board_identifier)

    try:
        analog_channels  = arr.array("i", [0])              # motor current sense (A)
        encoder_channels = arr.array("i", [0, 1])           # motor encoder, pendulum encoder (counts)
        digital_channels = arr.array("i", [0, 1, 2])        # amplifier fault, stall detected, stall error
        other_channels   = arr.array("i", [14000])          # tachometer (counts/sec)

        num_analog_channels  = len(analog_channels)
        num_encoder_channels = len(encoder_channels)
        num_digital_channels = len(digital_channels)
        num_other_channels   = len(other_channels)

        voltages = arr.array("d", [0.0] * num_analog_channels)
        counts   = arr.array("i", [0] * num_encoder_channels)
        states   = arr.array("b", [0] * num_digital_channels)
        values   = arr.array("d", [0.0] * num_other_channels)

        card.read(analog_channels, num_analog_channels, 
                  encoder_channels, num_encoder_channels,
                  digital_channels, num_digital_channels,
                  other_channels, num_other_channels,
                  voltages, 
                  counts, 
                  states, 
                  values)

        for channel in range(num_analog_channels) :
            print("ADC #%d: %7.4f" % (analog_channels[channel], voltages[channel]), end = "   ")
        print()
        for channel in range(num_encoder_channels) :
            print("ENC #%d: %7d" % (encoder_channels[channel], counts[channel]), end = "   ")
        print()
        for channel in range(num_digital_channels) :
            print("DIG #%d: %7d" % (digital_channels[channel], states[channel]), end = "   ")
        print()
        for channel in range(num_other_channels) :
            print("ADC #%d: %7.4f" % (other_channels[channel], values[channel]), end = "   ")
        print()

    except HILError as ex:
        print("Unable to read channels. %s" % ex.get_error_message())

    finally:
        card.close()

except HILError as ex:
    print("Unable to open board. %s" % ex.get_error_message())

input("Press Enter to continue.")
exit(0)
