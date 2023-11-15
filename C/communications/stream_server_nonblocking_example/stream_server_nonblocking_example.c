/**************************************************************************
 *
 * stream_server_nonblocking_example.c - C file
 *
 * Quanser Stream C API Server Non-Blocking I/O Example.
 * Communication Loopback Demo.
 * 
 * This example uses the Quanser Stream C functions to accept connections 
 * from Simulink client models and send/receive data to/from them. This
 * application uses non-blocking I/O for sending/receiving data.
 *
 * This example demonstrates the use of the following functions:
 *    stream_listen
 *    stream_poll
 *    stream_accept
 *    stream_receive_double
 *    stream_send_double
 *    stream_flush
 *    stream_close
 *
 * Copyright (C) 2012 Quanser Inc.
 *
 **************************************************************************/

#include "stream_server_nonblocking_example.h"

static int stop = 0;

void signal_handler(int signal)
{
    stop = 1;
}

int main(int argc, char * argv[])
{
    const char      uri[] = "tcpip://localhost:18000";
//  const char      uri[] = "udp://localhost:18000";
//  const char      uri[] = "shmem://foobar:1";
    const t_boolean nonblocking = true;
    const t_int     send_buffer_size = 8000;
    const t_int     receive_buffer_size = 8000;
    const char *    locale = NULL;

    qsigaction_t action;
    t_stream server;
    t_stream client;
    t_error result;

    unsigned long count;
    unsigned long iterations;
    t_double value;

    char message[512];

    /* Catch Ctrl+C so application may shut down cleanly */
    action.sa_handler = signal_handler;
    action.sa_flags   = 0;
    qsigemptyset(&action.sa_mask);
    
    qsigaction(SIGINT, &action, NULL);
    
    printf("Quanser Stream Server Non-Blocking I/O Example\n\n");
    printf("Press Ctrl+C to stop\n\n");
    printf("Listening on URI '%s'...\n", uri);
    
    /* This function establishes a server stream which listens on the given URI. */
    result = stream_listen(uri, nonblocking, &server);
    if(result == 0)
    {
        char wait_symbols[] = { '|', '/', '-', '\\' };
        int wait_symbol_index = 0;
        t_timeout timeout;
        
        timeout.seconds     = 0;
        timeout.nanoseconds = 30000000; /* 0.030 seconds */
        timeout.is_absolute = false;
        
        /*
         * Continue to accept client connections, one at a time, so that the server
         * does not stop running after the first client closes their connection.
         */
        while (!stop)
        {
            /* Print waiting message with spinning wait symbol */
            printf("Waiting for a new connection from a client: %c\r", wait_symbols[wait_symbol_index]);
            wait_symbol_index = (wait_symbol_index + 1) % ARRAY_LENGTH(wait_symbols);

            /*
             * Poll the server stream to determine if there are any pending client
             * connections. Wait up to 0.2 seconds and then timeout. Note that the
             * stream_poll function can poll for more than one condition at the
             * same time, as the last argument is a bitmask. It returns a bitmask
             * indicating which conditions were satisfied. If it returns zero then
             * it timed out before any of the events occurred. If is is negative then
             * an error occurred.
             *
             * Also, for connectionless protocols like UDP the stream_poll call will
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
            result = stream_poll(server, &timeout, STREAM_POLL_ACCEPT);
            if (result < 0) /* then an error occurred */
                break;
            else if (result == STREAM_POLL_ACCEPT) /* then connection pending */
            {
                enum states
                {
                    STATE_RECEIVE,
                    STATE_SEND,
                    STATE_FLUSH
                } state = STATE_RECEIVE;
                
                /*
                 * A client connection is pending. Accept the connection and proceed to commmunicate
                 * with the client. See notes above on the stream_poll invocation for the behaviour
                 * of UDP.
                 */
                result = stream_accept(server, send_buffer_size, receive_buffer_size, &client);
                if (result < 0)
                    break;
                    
                printf("\nAccepted a connection from a client\n");
                printf("Sending and receiving data.\n\n");
                
                iterations = 0;
                count = 0;

                while (!stop)
                {
                    /*
                     * Use a simple state machine to communicate with the client. First receive a value from the client.
                     * Then send that value back to the client and finally flush the stream so that the data is sent
                     * immediately rather than waiting for the stream send buffer to become full first. A state machine
                     * is used because the stream functions could return -QERR_WOULD_BLOCK at any stage, indicating that
                     * it was not possible to complete the operation without blocking. In that case the state does not
                     * change and it attempts to do the operation again the next time around the loop.
                     */
                    switch (state)
                    {
                        case STATE_RECEIVE:
                            /* 
                             * Attempt to receive a double value from the server. If the result is zero then the server closed the
                             * connection gracefully and this end of the connection should also be closed. If the result is negative
                             * then an error occurred and the connection should be closed as well, unless the error is -QERR_WOULD_BLOCK.
                             * The QERR_WOULD_BLOCK error is not a failure condition but indicates that the double value could not be
                             * received without blocking. In this case, more attempts to receive the value may be done or other operations
                             * may be performed.
                             *
                             * Note that UDP is a connectionless protocol so normally the server would not be able to detect if the
                             * client closed its "connection". However, the stream API sends a zero-length datagram by default when a UDP 
                             * "connection" is closed so that the server can detect it (assuming the packet isn't lost enroute).
                             * Transmission of a zero-length datagram on closure can be disable via the UDP options in the URI.
                             * See the stream API documentation on the UDP protocol URI for details.
                             *
                             * The stream_receive_double function never reads only some of the bytes of the double from the stream.
                             * It either retrieves an entire double from the stream or nothing. In other words, the double value is
                             * treated as an atomic unit. These semantics make it much easier to receive multibyte data types using
                             * non-blocking I/O. Using the stream_peek set of functions, it is even possible to retrieve complex
                             * data types. Also refer to the stream_set_swap_bytes function to account for byte-order differences
                             * between the client and server.
                             */
                            result = stream_receive_double(client, &value);
                            if (result <= 0) /* then an error occurred, it would have blocked or the stream was closed at the peer */
                                break;
                            
                            printf("Value: %6.3lf\r", value);
                            
                            /* Count the number of values received */
                            count++;
                            
                            /* Flag that the value should be sent back to the client */
                            state = STATE_SEND;
                            
                            /* Fall through deliberately so the send is attempted right away */
                    
                        case STATE_SEND:
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
                            if (result < 0) /* then an error occurred or it would have blocked */
                                break;

                            state = STATE_FLUSH;
                            
                            /* Fall through deliberately so the flush is attempted right away */
                            
                        case STATE_FLUSH:
                            /* 
                             * To ensure that the data is transmitting immediately, in order to minimize latency at the
                             * possible expense of throughput, flush the stream, as done in this example below.
                             */
                            result = stream_flush(client);
                            if (result == 0)
                            {
                                state = STATE_RECEIVE;
                                result = 1; /* prevent (result <= 0) test below from causing loop to exit */
                            }
                            break;
                    }

                    /*
                     * If an error occurred or the peer closed their end of the connection then exit the loop
                     * and close the client connection.
                     */
                    if (result <= 0 && result != -QERR_WOULD_BLOCK)
                        break;

                    /* Count the number of iterations of the loop */
                    iterations++;
                }
                
                stream_close(client);
                printf("\n\nConnection closed. Items processed: %lu. Number of iterations: %lu\n\n", count, iterations);
            }
        }
        
        if (result < 0 && result != -QERR_WOULD_BLOCK)
        {
            msg_get_error_messageA(locale, result, message, sizeof(message));
            printf("Unable to accept conenctions on URI '%s'. %s\n", uri, message);
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
    
    printf("\n\nPress Enter to exit\n");
    getchar();

    return 0;
}

