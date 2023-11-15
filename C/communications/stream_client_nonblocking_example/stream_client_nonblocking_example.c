/*********************************************************************
 *
 * stream_client_nonblocking_example.c - C file
 *
 * Quanser Stream C API Client Non-Blocking I/O Example.
 * Communication Loopback Demo.
 * 
 * This example uses the Quanser Stream C functions to connect 
 * to Simulink server models and send/receive data to/from them. This
 * application uses non-blocking I/O for sending/receiving data.
 *
 * NOTE: This example MUST be run prior to the server when using
 *       a connectionless protocol such as UDP.
 *
 * This example demonstrates the use of the following functions:
 *    stream_connect
 *    stream_poll
 *    stream_receive_double
 *    stream_send_double
 *    stream_flush
 *    stream_close
 *
 * Copyright (C) 2012 Quanser Inc.
 *
 *********************************************************************/

#include "stream_client_nonblocking_example.h"

static int stop = 0;

void signal_handler(int signal)
{
    stop = 1;
}

int main(int argc, char * argv[])
{
  	const char      uri[]               = "tcpip://localhost:18000";
//	const char      uri[]               = "udp://localhost:18000";
//  const char      uri[]               = "shmem://foobar:1";
    const t_boolean nonblocking         = true;
    const t_int     send_buffer_size    = sizeof(t_double);
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

    printf("Quanser Stream Client Non-Blocking I/O Example\n\n");
    printf("Press Ctrl+C to stop\n\n");
    printf("Connecting to URI '%s'...\n", uri);

    /*
     * This function attempts to connect to the server using the specified URI. Because
     * non-blocking I/O has been selected, it will return immediately with either success,
     * or an error code. If the error is -QERR_WOULD_BLOCK then a connection is in progress
     * and the stream returned in the client argument is valid. In that case the stream_poll
     * function must be used with the STREAM_POLL_CONNECT flag to poll for the completion
     * of the connection.
     *
     * Note that for connectionless protocols such as UDP, the stream_connect will return
     * successfully even without a "connection". The stream_connect in this case merely
     * establishes the address to which datagrams will be sent.
     */
    result = stream_connect(uri, nonblocking, send_buffer_size, receive_buffer_size, &client);
    if (result == -QERR_WOULD_BLOCK) /* connection in progress */
    {
        t_timeout timeout;
        
        /* 
         * Make the timeout a relative timeout of 10 seconds. Note that the stream API supports
         * absolute timeouts as well using the same structure, but setting the is_absolute
         * flag. Use the functions in quanser_time.h for getting the current absolute time
         * and manipulating t_timeout structures (e.g. adding, subtracting, comparing, etc).
         */
        timeout.seconds     = 10;
        timeout.nanoseconds = 0;
        timeout.is_absolute = false;

        /* 
         * Wait for the connection to complete by invoking stream_poll with the
         * STREAM_POLL_CONNECT flag. Note that stream_poll can be used with multiple
         * flags at the same time (as a bitmask) and returns a bitmask representing
         * the conditions that were satisfied. However, in this case there is only one
         * flag so bitwise operations are not required.
         *
         * If the timeout expires before the requested event occurs then stream_poll
         * returns zero. In this case, we change the result to -QERR_TIMED_OUT so
         * that we can simply fall through to subsequent code and let the normal
         * error handling process the error. The stream_poll function does not
         * return -QERR_TIMED_OUT directly because timing out is not an error condition
         * and stream_poll is used in many other instances where a zero return value
         * in the case of a timeout is easier to integrate into code.
         */
        result = stream_poll(client, &timeout, STREAM_POLL_CONNECT);
        if (result == 0) /* timed out waiting to connect */
            result = -QERR_TIMED_OUT;
    }

    if (result >= 0)
    {
        const t_double amplitude = 5.0;
        const t_double time_scale = 0.1;
        unsigned long count = 0;
        unsigned long iterations = 0;
        t_double value = 0.0;
        t_boolean no_data_received = true;
        t_boolean do_send = true;

        /* The client is now connected */
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
             * The stream_send_double call may return -QERR_WOULD_BLOCK, indicating that the data could not 
             * be sent at this time without blocking. The stream_poll function could be used with the 
             * STREAM_POLL_SEND flag to ensure that at least one byte of space is available in the stream's 
             * send buffer (although stream_send_double could still return -QERR_WOULD_BLOCK attempting to 
             * write an 8-byte double value). To ensure the data is sent, do a loop in which stream_poll is 
             * called first (with a timeout or NULL to block indefinitely) followed by the stream_send_double.
             * Note that stream_send_double NEVER writes part of the double into the stream's send buffer. 
             * It always treats the complete double as a single atomic unit. In other words, it writes the
             * entire double into the stream buffer or none of it.
             *
             * For example, to block for a certain timeout period (relative_timeout) attempting to send
             * the data, use the following code:
             *
             *    t_timeout absolute_timeout;
             *
             *    result = timeout_get_absolute(&absolute_timeout, &relative_timeout);
             *    if (result == 0) {
             *        do {
             *            result = stream_poll(client, &absolute_timeout, STREAM_POLL_SEND);
             *            if (result == STREAM_POLL_SEND) {
             *                result = stream_send_double(client, value);
             *            else if (result == 0) // timed out
             *                result = -QERR_TIMED_OUT;  // issue an error since peer not responding
             *        } while (result == -QERR_WOULD_BLOCK);
             *    }
             *
             * Note that the timeout is converted to absolute so that the loop exits within the original
             * timeout regardless how many times the loop actually executes. The loop will only execute
             * more than once if the stream's send buffer has bytes free but not enough for a complete
             * double value and the data cannot be transmitted immediately via the underlying communication
             * channel.
             *
             * To make non-blocking I/O act like blocking I/O then specify a NULL timeout to the stream_poll, 
             * which indicates an infinite timeout.
             *
             * A similar loop can be done for the stream_flush using stream_poll with the STREAM_POLL_FLUSH
             * flag. If data is being flushed after every send operation then the above send loop is unnecessary
             * if the size of the stream send buffer specified in stream_connect is large enough to hold all
             * the data because in that case it is known in advance that the stream_send_double will never
             * block. Flushing the stream transmits the data in the stream's send buffer to the underlying
             * communication channel and removes it from the stream's send buffer.
             *
             * For packet-based protocols like TCP/IP the loop may not be necessary either if the data is
             * being flushed every time and is known to be small enough to fit within a single packet. For
             * example, with TCP/IP it is possible to send 1460 bytes in every packet. Hence, for our
             * double, which is only eight bytes, the code could look like the following:
             *
             *     result = stream_send_double(client, value);
             *     if (result > 0) {
             *         result = stream_poll(client, &timeout, STREAM_POLL_FLUSH); // underlying channel has space
             *         if (result == STREAM_POLL_FLUSH)
             *             result = stream_flush(client); // all data will be sent since it fits in one packet
             *         else if (result == 0) // timed-out. Peer not reading data?
             *             result = -QERR_TIMED_OUT;
             *     }
             *
             * A sawtooth waveform is sent to the server. Note that the stream's send buffer was set to the
             * size of a single double in the stream_connect call to prevent a large collection of data points
             * from accumulating in the send buffer if the stream_flush kept returning -QERR_WOULD_BLOCK. In
             * other applications such accumulation may be desirable. Note that the stream_flush sends as much
             * data from the stream's send buffer as it can, so all data in the stream's send buffer will be
             * transmitted eventually. The stream_flush will return -QERR_WOULD_BLOCK even if it manages to
             * send some of the data. It only returns zero if all the data is flushed and the stream's send
             * buffer is now empty. Also note that stream_send_double itself will attempt to flush the stream
             * if there is no more space in the stream's send buffer for data.
             *
             * For protocols like UDP in which packets are not acknowledged, this example will keep firing
             * off packets whether the server exists or not, because it does not know if the packets are
             * received. Doing so means that the server may be started after the client, but this behaviour
             * may not be desirable depending on the application. Once the server has responded, we set the
             * no_data_received flag to false so that data is no longer sent indiscriminately, but waits for
             * a data point to be received before sending each value. The do_send flag is used to keep track
             * of each time a data point is received so that only one value is sent for each value received.
             * Note that multiple zero values may be transmitted for any protocol before the first value is
             * received back from the server because of the time required for the server to respond to the
             * first packet. Remember that because non-blocking I/O is being used in this example none of
             * the functions below actually block - all of them return quickly. This fact is readily apparent
             * from the iterations count printed when the example exits. Note that the iteration count will
             * be extremely large if the printf issuing the last value received is commented out.
             *
             * For UDP, the server requires that a packet be received from a client before it can respond
             * because it is a connectionless protocol and otherwise would not know to whom data should be
             * sent. See one of the server examples for details.
             */
            if (do_send || no_data_received)
            {
                result = stream_send_double(client, fmod(time_scale * count, amplitude));
                if (result > 0)
                {
                    /* 
                     * To ensure that the data is transmitting immediately, in order to minimize latency at the
                     * possible expense of throughput, flush the stream, as done in this example below. Note
                     * that stream_flush may return -QERR_WOULD_BLOCK. See the comment above on stream_send_double
                     * for possible ways of handling this return value. In this case, we ignore the error and
                     * allow a new value to be received if one is available. However, since the stream_send_double
                     * succeeded above, the value is in the stream's send buffer and will be transmitted the next
                     * time data is flushed to the underlying communication channel.
                     */
                    result = stream_flush(client);
                    if (result == 0) /* all the data was flushed */
                    {
                        if (!no_data_received)
                            count++; /* count number of doubles transmitted after the server has responded */
                        
                        do_send = false; /* don't send again until next value received from server */
                    }
                    else if (result < 0 && result != -QERR_WOULD_BLOCK)
                        break;
                }
                else if (result != -QERR_WOULD_BLOCK)
                    break;
            }
            
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
             *
             * In this example the stream_send is done first so the connection will be bound to the UDP port already, 
             * since the first stream_send will not block. Note that it is actually the stream_flush after the stream_send
             * that binds the UDP port because the stream_send may not access the underlying communication channel if
             * the stream's send buffer is not full.
             */
            result = stream_receive_double(client, &value);
            if (result > 0)
            {
                /*
                 * Print out the value received. Also slows down the loop a bit so it doesn't run ridiculously fast.
                 */
                printf("Value: %6.3lf\r", value);
                
                /* Flag that we have received data, which indicates the server has responded */
                no_data_received = false;
                
                /* Flag that a new data point should be sent */
                do_send = true;

            }
            else if (result != -QERR_WOULD_BLOCK) /* then a genuine error occurred or the connection was closed by the peer */
                break;

            /********************************************************************************************* 
             * Do other processing here. This example allows 100% of the CPU time to be used by this loop
             * (since the I/O never blocks) so that the iterations count shows how often the loop is run.
             *********************************************************************************************/

            /* Count the number of iterations (will be much higher than number of data items transmitted) */
            iterations++;
        }

        /* The connection has been closed at the server end or an error has occurred. Close the client stream. */
        stream_close(client);
        printf("\n\nConnection closed. Items processed: %lu. Number of iterations: %lu\n\n", count, iterations);

        /*
         * Check to see if an error occurred while sending/receiving.
         * In case of an error, print the appropriate message.
         */
        if (result < 0 && result != -QERR_WOULD_BLOCK)
        {
            msg_get_error_messageA(locale, result, message, sizeof(message));
            printf("Error communicating on URI '%s'. %s\n", uri, message);
        }
    }
    else /* stream_connect or stream_poll failed */
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

