//////////////////////////////////////////////////////////////////
//
// hil_get_string_property_example.c - C file
//
// This example gets the string propery (serial number) from a device.
//
// This example demonstrates the use of the following functions:
//    hil_open
//    hil_get_hil_string_property
//    hil_close
//
// Copyright (C) 2021 Quanser Inc.
//////////////////////////////////////////////////////////////////
    
#include "hil_get_string_property_example.h"

int main(int argc, char* argv[])
{
    static const char  board_type[]       = "qube_servo2_usb";
    static const char  board_identifier[] = "0";
    static char        message[512];

    t_card board;
    t_int  result;

    if (argc > 1)
        if (argc > 2)
            result = hil_open(argv[1], argv[2], &board);
        else
            result = hil_open(argv[1], board_identifier, &board);
    else
        result = hil_open(board_type, board_identifier, &board);

    if (result == 0)
    {
        char serial_number[32];

        result = hil_get_string_property(board, PROPERTY_STRING_SERIAL_NUMBER, serial_number, 32);
        if (result >= 0)
        {
            printf("Serial number for %s: %s\n", board_type, serial_number);
        }
        else
        {
            msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
            printf("Unable to read serial number. %s Error %d.\n", message, -result);
        }
            
        hil_close(board);
    }
    else
    {
        msg_get_error_message(NULL, result, message, ARRAY_LENGTH(message));
        printf("Unable to open board. %s Error %d.\n", message, -result);
    }

    return 0;
}
