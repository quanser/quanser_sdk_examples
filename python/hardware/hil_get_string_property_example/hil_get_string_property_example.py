#################################################################
#
# hil_get_string_property_example.py - Python file
#
# This example gets the string propery (serial number) from a device.
#
# This example demonstrates the use of the following functions:
#    HIL.open
#    HIL.get_hil_string_property
#    HIL.close
#
# Copyright (C) 2023 Quanser Inc.
#################################################################

from quanser.hardware import HIL, HILError, StringProperty
import array as arr
import sys

board_type = "qube_servo2_usb"
board_identifier = "0"

argc = len(sys.argv)
if argc > 1:
    board_type = sys.argv[1]
    if argc > 2:
        board_identifier = sys.argv[2]

try:
    card = HIL()
    card.open(board_type, board_identifier)

    try:
        serial_number = card.get_string_property(StringProperty.SERIAL_NUMBER, 64)
        print("Serial number for %s: %s" % (board_type, serial_number))

    except HILError as ex:
        print("Unable to get serial number. %s" % ex.get_error_message())

    finally:
        card.close()

except HILError as ex:
    print("Unable to open board. %s" % ex.get_error_message())

input("Press Enter to continue.")
exit(0)
