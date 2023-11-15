/**************************************************************************
 *
 * stream_server_blocking_example.c - C file
 *
 * Quanser Stream C API Server Blocking I/O Example.
 * Communication Loopback Demo.
 * 
 * This example uses the Quanser Stream C functions to accept connections 
 * from Simulink client models and send/receive data to/from them. This
 * application uses blocking I/O for sending/receiving data.
 *
 * This example demonstrates the use of the following functions:
 *    stream_listen
 *    stream_accept
 *    stream_receive_double
 *    stream_send_double
 *    stream_flush
 *    stream_close
 *
 * Copyright (C) 2012 Quanser Inc.
 *
 **************************************************************************/

#include "stream_server_blocking_example.h"

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

    printf("Quanser Stream Server Blocking I/O Example\n\n");
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
                unsigned long count = 0;
                t_double value = 0.0;

                printf("Accepted a connection from a client.\n");
                printf("Sending and receiving data.\n\n");

                /* 
                 * The client is now connected. Send and receive data in a loop.
                 */
                while (!stop)
                {
                    /* 
                     * Attempt to receive a double value from the server. If the result is zero then the client closed the
                     * connection gracefully and this end of the connection should also be closed. If the result is negative
                     * then an error occurred and the connection should be closed as well. Note that UDP is a connectionless
                     * protocol so normally the server would not be able to detect if the client closed its "connection".
                     * However, the stream API sends a zero-length datagram by default when a UDP "connection" is closed so that
                     * the server can detect it (assuming the packet isn't lost enroute). Transmission of a zero-length
                     * datagram on closure can be disable via the UDP options in the URI. See the stream API documentation
                     * on the UDP protocol URI for details.
                     */
                    result = stream_receive_double(client, &value);
                    if (result <= 0)
                        break;
                    
                    printf("Value: %6.3lf\r", value);
                    
                    /* Count the number of values received */
                    count++;
                    
                    /*
                     * "Send" a double value to the client. The stream_send_double function stores the double value
                     * in the stream's send buffer, but does not necessarily transmit the contents of the send buffer
                     * to the underlying communication channel. It only transmits the data if the send buffer becomes
                     * full or the stream is flushed. This paradigm is used to maximize throughput by allowing data 
                     * to be coalesced into fewer packets in packet-based protocols like TCP/IP and UDP/IP.
                     *
                     * This particular example simply echoes the received value back to the client who connected.
                     */
                    result = stream_send_double(client, value);
                    if (result < 0)
                        break;

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


