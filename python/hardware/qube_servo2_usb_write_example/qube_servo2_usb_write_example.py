#################################################################
#
# qube_servo2_usb_write_example.py - Python file
#
# This example writes one sample immediately to all the outputs
# of the Qube Servo2 USB.
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
import math
import time
import array as arr

board_type = "qube_servo2_usb"
board_identifier = "0"

try:
    card = HIL()
    card.open(board_type, board_identifier)

    try:
        analog_channels  = arr.array("i", [0])                      # motor voltage (V)
        digital_channels = arr.array("i", [0])                      # motor enable
        other_channels   = arr.array("i", [11000, 11001, 11002])    # LED red, green and blue

        num_analog_channels  = len(analog_channels)
        num_digital_channels = len(digital_channels)
        num_other_channels   = len(other_channels)

        voltages = arr.array("d", [1.0] * num_analog_channels)  # 1V to motor
        states   = arr.array("b", [1] * num_digital_channels)   # enable motor
        values   = arr.array("d", [1.0, 0.5, 0.2])              # red, green, blue components

        print("This example moves the motor for one second and changes the LED")
        print("colour. The values written are::\n")
        for channel in range(num_analog_channels):
            print("    DAC[%d] = %4.1f V" % (analog_channels[channel], voltages[channel]))
        print()
        for channel in range(num_digital_channels):
            print("    DIG[%d] = %d" % (digital_channels[channel], states[channel]))
        print()
        for channel in range(num_other_channels):
            print("    OTH[%d] = %1.2f" % (other_channels[channel], values[channel]))
        print()

        card.set_digital_directions(None, 0, digital_channels, num_digital_channels)
        card.write(analog_channels, num_analog_channels, 
                   None, 0,
                   digital_channels, num_digital_channels,
                   other_channels, num_other_channels,
                   voltages, 
                   None, 
                   states, 
                   values)

        # Sleep for one second
        time.sleep(1)

        # Stop the motor and turn the LED red
        voltages[0] = 0.0   # set motor voltage to zero
        states[0]   = 0     # turn motor off
        values[0]   = 1     # pure red
        values[1]   = 0
        values[2]   = 0

        card.write(analog_channels, num_analog_channels, 
                   None, 0,
                   digital_channels, num_digital_channels,
                   other_channels, num_other_channels,
                   voltages, 
                   None, 
                   states, 
                   values)

    except HILError as ex:
        print("Unable to write channels. %s" % ex.get_error_message())

    finally:
        card.close()

except HILError as ex:
    print("Unable to open board. %s" % ex.get_error_message())

input("Press Enter to continue.")
exit(0)
