//////////////////////////////////////////////////////////////////
//
//	stream_server_mixed_types_blocking_example.h - header file
//
//////////////////////////////////////////////////////////////////

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#include <stdio.h>
#include <math.h>

#include "quanser_stream.h"		/* Include Quanser Stream API header file */
#include "quanser_messages.h"	/* Include header for getting error messages */
#include "quanser_signal.h"     /* Include signal handling functions */
