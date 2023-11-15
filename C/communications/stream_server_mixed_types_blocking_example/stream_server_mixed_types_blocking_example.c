/**************************************************************************
 *
 * stream_server_mixed_types_blocking_example.c - C file
 *
 * Quanser Stream C API Server Mixed Types Blocking I/O Example.
 * Communication Loopback Demo.
 * 
 * This example uses the Quanser Stream C functions to accept connections 
 * from Simulink client models and send/receive data to/from them. This
 * application uses blocking I/O for sending/receiving data.
 *
 * This example demonstrates the use of the following functions:
 *    stream_listen
 *    stream_accept
 *    stream_peek_begin
 *    stream_peek_byte_array
 *    stream_peek_short
 *    stream_peek_end
 *    stream_poke_begin
 *    stream_poke_double
 *    stream_poke_byte
 *    stream_poke_single_array
 *    stream_poke_end
 *    stream_flush
 *    stream_close
 *
 * Copyright (C) 2012 Quanser Inc.
 *
 **************************************************************************/

#include "stream_server_mixed_types_blocking_example.h"

static int stop = 0;

void signal_handler(int signal)
{
    stop = 1;
}

int main(int argc, char * argv[])
{
  	const char      uri[] = "tcpip://localhost:18000";
//	const char      uri[] = "udp://localhost:18000";
//  const char      uri[] = "shmem://foobar:1";
    const t_boolean nonblocking = false;
    const t_int		send_buffer_size = 8000;
    const t_int		receive_buffer_size = 8000;
    const char *	locale = NULL;

    qsigaction_t action;
    t_stream server;
    t_stream client;
    t_error result;

    char message[512];

    /* Catch Ctrl+C so application may shut down cleanly */
    action.sa_handler = signal_handler;
    action.sa_flags   = 0;
    qsigemptyset(&action.sa_mask);

    qsigaction(SIGINT, &action, NULL);

    printf("Quanser Stream Server Mixed Types Blocking I/O Example\n\n");
    printf("Press Ctrl+C to stop (when client connected)\n\n");
    printf("Listening on URI '%s'...\n", uri);

    /* This function establishes a server stream which listens on the given URI. */
    result = stream_listen(uri, nonblocking, &server);
    if (result == 0)
    {
        /*
         * Continue to accept client connections, one at a time, so that the server
         * does not stop running after the first client closes their connection.
         */
        while (!stop)
        {
            printf("Waiting for a new connection from a client...\n");

            /*
             * This function accepts new connections from clients using the server 
             * stream created by stream_listen. Upon successful operation, it creates 
             * a client stream to be used for communications. For connectionless protocols
             * like UDP it returns immediately and the peer for communications is determined
             * by the first client to send data to the server (see stream API documentation on
             * the UDP protocol URI for options with respect to subsequent clients).
             *
             * Note that stream_poll may be used with the STREAM_POLL_ACCEPT flag and a
             * timeout to poll the stream for any pending client connections. Doing so
             * would allow the Ctrl+C to be detected on Windows without having a client 
             * connect.
             *
             * Also, for connectionless protocols like UDP the stream_accept call will
             * return immediately as if a client had connected. The client to whom
             * communications should be established is then determined by the first client
             * to send data to the server. Thus, a stream_receive operation should always
             * be done first for the UDP protocol. If a stream_send is attempted prior to
             * a stream_receive then -QERR_NO_DESIGNATED_PEER will be returned, indicating
             * that the server does not know to whom data should be sent because no data
             * has been received from a client. This error is not fatal and simply indicates
             * that a stream_receive must be done to determine the peer before data can be
             * sent. This restriction only applies to connectionless protocols like UDP.
             *
             * How subsequent clients to send data to the server are handled depends on
             * the options specified in the UDP URI.
             */
            result = stream_accept(server, send_buffer_size, receive_buffer_size, &client);
            if (result == 0)
            {
                const t_double pi = 3.1415926535897932384626433832795;
                const t_double amplitude = 2.5;
                const t_double time_scale = 0.01;
                unsigned long count = 0;

                printf("Accepted a connection from a client.\n");
                printf("Sending and receiving data.\n\n");

                /* 
                 * The client is now connected. Send and receive data in a loop.
                 */
                while (!stop)
                {
                    t_stream_poke_state poke_state;
                    t_stream_peek_state peek_state;
                    t_int8 bytes[3];
                    t_uint16 word;
                    t_single float_values[3];
                    t_double value;

                    /* 
                     * Attempt to receive mixed data types from the client. If the result is zero then the client closed the
                     * connection gracefully and this end of the connection should also be closed. If the result is negative
                     * then an error occurred and the connection should be closed as well. Note that UDP is a connectionless
                     * protocol so normally the server would not be able to detect if the client closed its "connection".
                     * However, the stream API sends a zero-length datagram by default when a UDP "connection" is closed so that
                     * the server can detect it (assuming the packet isn't lost enroute). Transmission of a zero-length
                     * datagram on closure can be disable via the UDP options in the URI. See the stream API documentation
                     * on the UDP protocol URI for details.
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
                     *    3 * sizeof(bytes[0]) + sizeof(t_int16)
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
                        result = stream_peek_byte_array(client, &peek_state, bytes, 3);
                        if (result > 0)
                            result = stream_peek_short(client, &peek_state, (t_int16 *) &word);

                        result = stream_peek_end(client, &peek_state, result);
                    }
                    if (result <= 0)
                        break;
                    
                    printf("Values: [%4d %4d %4d], %5u\r", bytes[0], bytes[1], bytes[2], word);
                    
                    /* Count the number of values received */
                    count++;
                    
                    /*
                     * "Send" multiple data types consisting of a double, byte and a single array to the client. 
                     * The stream_poke_begin function begins the process of writing mixed data types atomically to the 
                     * stream's send buffer. In this case, the stream_poke_double, stream_poke_byte and 
                     * stream_poke_single_array functions  are then used to write the actual data to the stream's send 
                     * buffer. Finally, the stream_poke_end function is called to complete the process. If any one of 
                     * the stream_poke functions fails then nothing is written to the stream's send buffer. The buffer 
                     * must be large enough to hold the data "poked" between the stream_poke_begin and stream_poke_end 
                     * call. The stream_poke_end call does not necessarily transmit the contents of the send buffer to 
                     * the underlying communication channel. It only transmits the data if the send buffer becomes full 
                     * or the stream is flushed. This paradigm is used to maximize throughput by allowing data to be 
                     * coalesced into fewer packets in packet-based protocols like TCP/IP and UDP/IP.
                     *
                     * If the result passed to stream_poke_end is less than one then it simply returns the value of result
                     * and the data is removed from the stream's send buffer and not transmitted. Otherwise, it adds the 
                     * data to the stream's send buffer and returns 1.
                     *
                     * A sawtooth waveform is written for the double value, a pulse train for the byte, and the single
                     * array consists of three sinusoidal waveforms of amplitude 1, 2 and 3 respectively.
                     */
                    result = stream_poke_begin(client, &poke_state);
                    if (result == 0)
                    {
                        result = stream_poke_double(client, &poke_state, fmod(count * time_scale, amplitude));
                        if (result > 0)
                        {
                            result = stream_poke_byte(client, &poke_state, fmod(count * time_scale, 1) > 0.5);
                            if (result > 0)
                            {
                                value = sin(2 * pi * time_scale * count);
                                float_values[0] = (t_single) (value);
                                float_values[1] = (t_single) (2 * value);
                                float_values[2] = (t_single) (3 * value);

                                result = stream_poke_single_array(client, &poke_state, float_values, 3);
                            }
                        }

                        result = stream_poke_end(client, &poke_state, result);
                    }

                    /* 
                     * To ensure that the data is transmitting immediately, in order to minimize latency at the
                     * possible expense of throughput, flush the stream, as done in this example below.
                     */
                    result = stream_flush(client);
                    if (result < 0)
                        break;
                }

                /* The connection has been closed at the server end or an error has occurred. Close the client stream. */
                stream_close(client);
                printf("\n\nConnection closed. Number of items: %lu\n", count);

                /* Go back to waiting for another client to connnect */
            }
            else /* a connection could not be accepted */
            {
                /* Print the appropriate message in case of errors in communication process. */
                msg_get_error_messageA(locale, result, message, sizeof(message));
                printf("Unable to accept connections on URI '%s'. %s\n", uri, message);
                break;
            }
        }
        
        /* The server session is ended. Close the server stream. */
        stream_close(server);
    }
    else /* failed to establish a listening stream */
    {
        /* If there was an error trying to create the server stream, print an error message and close the stream. */
        msg_get_error_messageA(locale, result, message, sizeof(message));
        printf("Unable to listen on URI '%s'. %s\n", uri, message);
    }
    
    printf("Press Enter to exit\n");
    getchar();

    return 0;
}
