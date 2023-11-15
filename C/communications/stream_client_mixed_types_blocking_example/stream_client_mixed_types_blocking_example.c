/*********************************************************************
 *
 * stream_client_mixed_types_blocking_example.c - C file
 *
 * Quanser Stream C API Client Mixed Types Blocking I/O Example.
 * Communication Loopback Demo.
 * 
 * This example uses the Quanser Stream C functions to connect 
 * to Simulink server models and send/receive data to/from them. This
 * application uses blocking I/O for sending/receiving data.
 *
 * This example demonstrates the use of the following functions:
 *    stream_connect
 *    stream_peek_begin
 *    stream_peek_double
 *    stream_peek_byte
 *    stream_peek_single_array
 *    stream_peek_end
 *    stream_poke_begin
 *    stream_poke_byte_array
 *    stream_poke_short
 *    stream_poke_end
 *    stream_flush
 *    stream_close
 *
 * Copyright (C) 2012 Quanser Inc.
 *
 *********************************************************************/

#include "stream_client_mixed_types_blocking_example.h"

static int stop = 0;

void signal_handler(int signal)
{
    stop = 1;
}

int main(int argc, char * argv[])
{
    const char      uri[]               = "tcpip://localhost:18000";
//  const char      uri[]               = "udp://localhost:18000";
//  const char      uri[]               = "shmem://foobar:1";
    const t_boolean nonblocking         = false;
    const t_int     send_buffer_size    = 8000;
    const t_int     receive_buffer_size = 8000;
    const char *    locale              = NULL;

    qsigaction_t action;
    t_stream client;
    t_error result;
    char message[512];

    /* Catch Ctrl+C so application may shut down cleanly */
    action.sa_handler = signal_handler;
    action.sa_flags   = 0;
    qsigemptyset(&action.sa_mask);

    qsigaction(SIGINT, &action, NULL);

    printf("Quanser Stream Client Mixed Types Blocking I/O Example\n\n");
    printf("Press Ctrl+C to stop\n\n");
    printf("Connecting to URI '%s'...\n", uri);

    /*
     * This function attempts to connect to the server using the specified URI. For UDP,
     * which is a connectionless protocol, this call will return immediately.
     */
    result = stream_connect(uri, nonblocking, send_buffer_size, receive_buffer_size, &client);
    if (result == 0) /* now connected */
    {
        const t_double pi = 3.1415926535897932384626433832795;
        const t_double time_scale = 0.01;
        unsigned long count = 0;
        
        printf("Connected to URI '%s'...\n\n", uri);

        /* The loop for sending/receving data to/from the server */
        while (!stop)
        {
            t_stream_poke_state poke_state;
            t_stream_peek_state peek_state;
            t_int8 bytes[3];
            t_single float_values[3];
            t_double value;

            /*
             * "Send" multiple data types consisting of a 3-element byte array and a short to the server. 
             * The stream_poke_begin function begins the process of writing mixed data types atomically to the 
             * stream's send buffer. In this case, the stream_poke_byte_array and stream_poke_short functions 
             * are then used to write the actual data to the stream's send buffer. Finally, the stream_poke_end 
             * function is called to complete the process. If any one of the stream_poke functions fails then 
             * nothing is written to the stream's send buffer. The buffer must be large enough to hold the data
             * "poked" between the stream_poke_begin and stream_poke_end call. The stream_poke_end call
             * does not necessarily transmit the contents of the send buffer to the underlying communication 
             * channel. It only transmits the data if the send buffer becomes full or the stream is flushed. 
             * This paradigm is used to maximize throughput by allowing data to be coalesced into fewer packets 
             * in packet-based protocols like TCP/IP and UDP/IP.
             *
             * If the result passed to stream_poke_end is less than one then it simply returns the value of result
             * and the data is removed from the stream's send buffer and not transmitted. Otherwise, it adds the 
             * data to the stream's send buffer and returns 1.
             *
             * The byte (uint8) array consists of three sinusoidal waveforms of amplitude 40, 80 and 120
             * respectively. A sawtooth waveform of amplitude 65535 is written as the short (uint16) value. 
             */
            result = stream_poke_begin(client, &poke_state);
            if (result == 0)
            {
                value = 40 * sin(2 * pi * time_scale * count);
                bytes[0] = (t_int8) (value);
                bytes[1] = (t_int8) (2 * value);
                bytes[2] = (t_int8) (3 * value);

                result = stream_poke_byte_array(client, &poke_state, bytes, 3);
                if (result > 0)
                    result = stream_poke_short(client, &poke_state, (t_int16) (1000 * count));

                result = stream_poke_end(client, &poke_state, result);
            }

            /* 
             * To ensure that the data is transmitting immediately, in order to minimize latency at the
             * possible expense of throughput, flush the stream, as done in this example below.
             */
            result = stream_flush(client);
            if (result < 0)
                break;
            
            /* 
             * Attempt to receive mixed data types from the server. If the result is zero then the server closed the
             * connection gracefully and this end of the connection should also be closed. If the result is negative
             * then an error occurred and the connection should be closed as well.
             *
             * The stream_peek_begin function begins the process of receiving mixed data types atomically from a
             * communication channel. In this case, the stream_peek_double, stream_peek_byte and stream_peek_single_array
             * functions are then used to receive the data from the stream. If any of the stream_peek functions
             * fails then none of the data is removed from the stream's receive buffer. The buffer must be large 
             * enough to hold the data "peeked" between the stream_peek_begin and stream_peek_end call. The
             * stream_peek_end call completes the process by removing the "peeked" data from the stream's receive
             * buffer is the previous operations were successful (i.e. if the result passed as an argument is 1).
             *
             * If the result passed to stream_peek_end is less than one then it simply returns the value of result
             * and the data remains in the stream's receive buffer. Otherwise, it removes the data from the stream's
             * receive buffer and returns 1.
             *
             * Note that for the UDP protocol the stream_connect does not explicitly bind the UDP socket to the port,
             * as the server does, because otherwise the client and server could not be run on the same machine. Hence,
             * the stream implicitly binds the UDP socket to the port when the first stream_send operation is performed.
             * Thus, a stream_send should always be executed prior to the first stream_receive for the UDP
             * protocol. If a stream_receive is attempted first, using UDP, then the -QERR_CONNECTION_NOT_BOUND
             * error will be returned by stream_receive. The error is not fatal and simply indicates that a send
             * operation must be performed before data can be received. This restriction only applies to
             * connectionless protocols like UDP.
             *
             * The number of bytes to prefetch passed to stream_peek_begin is zero. It could be:
             *    sizeof(value) + sizeof(bytes[0]) + 3 * sizeof(float_values[0])
             * in order to ensure that the required number of bytes is prefetched, even for protocols like I2C
             * that do not support read-ahead on the stream. However, protocols support read-ahead by default
             * so most protocols will attempt to prefetch as much data as possible any time a peek or receive
             * operation is performed. Hence, a prefetch of zero is an efficient and acceptable value for
             * most protocols. Using a value of zero for protocols which do not support read-ahead, like I2C,
             * will still work but will be less efficient than specifying the number of bytes to prefetch.
             */
            result = stream_peek_begin(client, &peek_state, 0);
            if (result > 0)
            {
                result = stream_peek_double(client, &peek_state, &value);
                if (result > 0)
                {
                    result = stream_peek_byte(client, &peek_state, &bytes[0]);
                    if (result > 0)
                        result = stream_peek_single_array(client, &peek_state, float_values, 3);
                }

                result = stream_peek_end(client, &peek_state, result);
            }

            if (result <= 0)
                break;

            printf("Values: %6.3lf, %d, [%6.3f %6.3f %6.3f]\r", 
                    value, bytes[0], float_values[0], float_values[1], float_values[2]);

            /* Count the number of values sent and received */
            count++;
        }

        /* The connection has been closed at the server end or an error has occurred. Close the client stream. */
        stream_close(client);
        printf("\n\nConnection closed. Number of items: %lu\n", count);

        /*
         * Check to see if an error occurred while sending/receiving.
         * In case of an error, print the appropriate message.
         */
        if (result < 0)
        {
            msg_get_error_messageA(locale, result, message, sizeof(message));
            printf("Error communicating on URI '%s'. %s\n", uri, message);
        }
    }
    else /* stream_connect failed */
    {
        /*
         * If the stream_connect function encountered an error, the client cannot connect
         * to the server. Print the message corresponding to the error that occured.
         */
        msg_get_error_messageA(locale, result, message, sizeof(message));
        printf("Unable to connect to URI '%s'. %s\n", uri, message);
    }
    
    printf("Press Enter to exit\n");
    getchar();

    return 0;
}
