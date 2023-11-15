/*********************************************************************
 *
 * stream_client_blocking_example.c - C file
 *
 * Quanser Stream C API Client Blocking I/O Example.
 * Communication Loopback Demo.
 * 
 * This example uses the Quanser Stream C functions to connect 
 * to Simulink server models and send/receive data to/from them. This
 * application uses blocking I/O for sending/receiving data.
 *
 * This example demonstrates the use of the following functions:
 *    stream_connect
 *    stream_receive_double
 *    stream_send_double
 *    stream_flush
 *    stream_close
 *
 * Copyright (C) 2012 Quanser Inc.
 *
 *********************************************************************/

#include "stream_client_blocking_example.h"

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

    printf("Quanser Stream Client Blocking I/O Example\n\n");
    printf("Press Ctrl+C to stop\n\n");
    printf("Connecting to URI '%s'...\n", uri);

    /*
     * This function attempts to connect to the server using the specified URI. For UDP,
     * which is a connectionless protocol, this call will return immediately.
     */
    result = stream_connect(uri, nonblocking, send_buffer_size, receive_buffer_size, &client);
    if (result == 0) /* now connected */
    {
        const t_double amplitude = 5.0;
        const t_double time_scale = 0.1;
        unsigned long count = 0;
        t_double value = 0.0;
        
        printf("Connected to URI '%s'...\n\n", uri);

        /* The loop for sending/receving data to/from the server */
        while (!stop)
        {
            /*
             * "Send" a double value to the server. The stream_send_double function stores the double value
             * in the stream's send buffer, but does not necessarily transmit the contents of the send buffer
             * to the underlying communication channel. It only transmits the data if the send buffer becomes
             * full or the stream is flushed. This paradigm is used to maximize throughput by allowing data 
             * to be coalesced into fewer packets in packet-based protocols like TCP/IP and UDP/IP.
             *
             * A sawtooth waveform is sent to the server.
             */
            result = stream_send_double(client, fmod(time_scale * count, amplitude));
            if (result < 0)
                break;
            
            /* 
             * To ensure that the data is transmitting immediately, in order to minimize latency at the
             * possible expense of throughput, flush the stream, as done in this example below.
             */
            result = stream_flush(client);
            if (result < 0)
                break;
            
            /* 
             * Attempt to receive a double value from the server. If the result is zero then the server closed the
             * connection gracefully and this end of the connection should also be closed. If the result is negative
             * then an error occurred and the connection should be closed as well.
             *
             * Note that for the UDP protocol the stream_connect does not explicitly bind the UDP socket to the port,
             * as the server does, because otherwise the client and server could not be run on the same machine. Hence,
             * the stream implicitly binds the UDP socket to the port when the first stream_send operation is performed.
             * Thus, a stream_send should always be executed prior to the first stream_receive for the UDP
             * protocol. If a stream_receive is attempted first, using UDP, then the -QERR_CONNECTION_NOT_BOUND
             * error will be returned by stream_receive. The error is not fatal and simply indicates that a send
             * operation must be performed before data can be received. This restriction only applies to
             * connectionless protocols like UDP.
             */
            result = stream_receive_double(client, &value);
            if (result <= 0)
                break;
            
            printf("Value: %6.3lf\r", value);

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
