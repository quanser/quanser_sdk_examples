/*********************************************************************
 *
 * stream_client_mixed_types_nonblocking_example.c - C file
 *
 * Quanser Stream C API Client Mixed Types Non-Blocking I/O Example.
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

#include "stream_client_mixed_types_nonblocking_example.h"

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
    const t_int     send_buffer_size    = 3 * sizeof(t_int8) + sizeof(t_uint16);
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

    printf("Quanser Stream Client Mixed Types Non-Blocking I/O Example\n\n");
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
        const t_double pi = 3.1415926535897932384626433832795;
        const t_double time_scale = 0.01;
        unsigned long count = 0;
        unsigned long iterations = 0;
        t_boolean no_data_received = true;
        t_boolean do_send = true;

        /* The client is now connected */
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
             *
             * The stream_poke calls may return -QERR_WOULD_BLOCK, indicating that the data could not 
             * be sent at this time without blocking. The stream_poll function could be used with the 
             * STREAM_POLL_SEND flag to ensure that at least one byte of space is available in the stream's 
             * send buffer (although stream_poke calls could still return -QERR_WOULD_BLOCK attempting to 
             * write data). To ensure the data is sent, do a loop in which stream_poll is called first
             * (with a timeout or NULL to block indefinitely) followed by the stream_poke_begin ... stream_poke_end
             * sequence. Note that the stream_poke sequence NEVER writes part of the data into the stream's send buffer. 
             * It always treats the everything between stream_poke_begin and stream_poke_end as a single atomic unit. 
             * In other words, it writes all data into the stream buffer or none of it.
             *
             * For example, to block for a certain timeout period (relative_timeout) attempting to send
             * the data, use the following code:
             *
             *    t_timeout absolute_timeout;
             *    t_stream_poke_state poke_state;
             *
             *    result = timeout_get_absolute(&absolute_timeout, &relative_timeout);
             *    if (result == 0) {
             *        do {
             *            result = stream_poll(client, &absolute_timeout, STREAM_POLL_SEND);
             *            if (result == STREAM_POLL_SEND) {
             *                result = stream_poke_begin(client, &poke_state);
             *                if (result == 0) {
             *                    // Call stream_poke functions to "poke" data into stream buffer
             *                    ...
             *                    result = stream_poke_end(client, &poke_state, result);
             *                }
             *            else if (result == 0) // timed out
             *                result = -QERR_TIMED_OUT;  // issue an error since peer not responding
             *        } while (result == -QERR_WOULD_BLOCK);
             *    }
             *
             * Note that the timeout is converted to absolute so that the loop exits within the original
             * timeout regardless how many times the loop actually executes. The loop will only execute
             * more than once if the stream's send buffer has bytes free but not enough for all the data
             * and the data cannot be transmitted immediately via the underlying communication channel.
             *
             * To make non-blocking I/O act like blocking I/O then specify a NULL timeout to the stream_poll, 
             * which indicates an infinite timeout.
             *
             * A similar loop can be done for the stream_flush using stream_poll with the STREAM_POLL_FLUSH
             * flag. If data is being flushed after every stream_poke_begin...stream_poke_end sequence then 
             * the above poke loop is unnecessary if the size of the stream send buffer specified in 
             * stream_connect is large enough to hold all the data because in that case it is known in advance
             * that the stream_poke sequence will never block. Flushing the stream transmits the data in the 
             * stream's send buffer to the underlying communication channel and removes it from the stream's 
             * send buffer.
             *
             * For packet-based protocols like TCP/IP the loop may not be necessary either if the data is
             * being flushed every time and is known to be small enough to fit within a single packet. For
             * example, with TCP/IP it is possible to send 1460 bytes in every packet. Hence, for our
             * double, which is only eight bytes, the code could look like the following:
             *
             *     result = stream_poke_begin(client, &poke_state);
             *     if (result == 0) {
             *         // Call stream_poke functions to "poke" data into stream buffer
             *         ...
             *         result = stream_poke_end(client, &poke_state, result);
             *     }
             *
             *     if (result > 0) {
             *         result = stream_poll(client, &timeout, STREAM_POLL_FLUSH); // underlying channel has space
             *         if (result == STREAM_POLL_FLUSH)
             *             result = stream_flush(client); // all data will be sent since it fits in one packet
             *         else if (result == 0) // timed-out. Peer not reading data?
             *             result = -QERR_TIMED_OUT;
             *     }
             *
             * Note that the stream's send buffer was set to the size of a three bytes and one short in the 
             * stream_connect call to prevent a large collection of data points from accumulating in the send 
             * buffer if the stream_flush kept returning -QERR_WOULD_BLOCK. In other applications such accumulation 
             * may be desirable. Note that the stream_flush sends as much data from the stream's send buffer 
             * as it can, so all data in the stream's send buffer will be transmitted eventually. The stream_flush
             * will return -QERR_WOULD_BLOCK even if it manages to send some of the data. It only returns zero if 
             * all the data is flushed and the stream's send buffer is now empty. Also note that stream_poke
             * functions above may also attempt to flush the stream if there is no more space in the stream's send 
             * buffer for data. However, the data in the process of being poked is never flushed until the
             * stream_poke_end call is made with a positive result argument.
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

                if (result > 0)
                {
                    /* 
                     * To ensure that the data is transmitting immediately, in order to minimize latency at the
                     * possible expense of throughput, flush the stream, as done in this example below. Note
                     * that stream_flush may return -QERR_WOULD_BLOCK. See the comment above on the stream_poke
                     * sequence for possible ways of handling this return value. In this case, we ignore the error and
                     * allow a new value to be received if one is available. However, since the stream_poke sequence
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
             * In this example the poke operation is done first so the connection will be bound to the UDP port already, 
             * since the first stream_poke sequence will not block. Note that it is actually the stream_flush after the 
             * stream_poke sequence that binds the UDP port because the stream_poke sequence may not access the underlying 
             * communication channel if the stream's send buffer is not full.
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

            if (result > 0)
            {
                /*
                 * Print out the values received. Also slows down the loop a bit so it doesn't run ridiculously fast.
                 */
                printf("Values: %6.3lf, %d, [%6.3f %6.3f %6.3f]\r", 
                        value, bytes[0], float_values[0], float_values[1], float_values[2]);
                
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

