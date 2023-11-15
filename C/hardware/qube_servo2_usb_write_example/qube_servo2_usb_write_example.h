//////////////////////////////////////////////////////////////////
//
//	qube_servo2_usb_write_example.h - header file
//
//////////////////////////////////////////////////////////////////

#include <stdio.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "hil.h"
#include "quanser_signal.h"
#include "quanser_messages.h"
#include "quanser_thread.h"

#ifdef _WIN32
#include <windows.h>
#define sleep Sleep
#define ONE_SECOND 1000
#else
#include "unistd.h"
#define ONE_SECOND 1
#endif // _WIN32

