######################################################################
#
# stream_client_nonblocking_example.py - Python file
#
# Quanser Stream Python API Client Non-Blocking I/O Example.
# Communication Loopback Demo.
#
# This example uses the Quanser Stream Python functions to connect
# to Simulink server models and send/receive data to/from them. This
# application uses non-blocking I/O for sending/receiving data.
#
# NOTE: This example MUST be run prior to the server when using
#       a connectionless protocol such as UDP.
#
# This example demonstrates the use of the following functions:
#    Stream.connect
#    Stream.poll
#    Stream.receive_doubles
#    Stream.send_double
#    Stream.flush
#    Stream.close
#
# Copyright (C) 2023 Quanser Inc.
#
######################################################################

from quanser.common import ErrorCode, Timeout
from quanser.communications import Stream, StreamError, PollFlag
import signal
import array as arr
import math

stop = False

def signal_handler(signum, frame):
    global stop
    stop = True

uri = "tcpip://localhost:18000"
#uri = "udp://localhost:18000"
#uri = "shmem://foobar:1"

nonblocking         = True
send_buffer_size    = 8000
receive_buffer_size = 8000

# Register a Ctrl+C handler
signal.signal(signal.SIGINT, signal_handler)

print("Quanser Stream Client Non-Blocking I/O Example\n")
print("Press Ctrl+C to stop\n")
print("Connecting to URI '%s'..." % uri)

#
# This function attempts to connect to the server using the specified URI. Because
# non-blocking I/O has been selected, it will return immediately with either success,
# or an error code. If the error is -ErrorCode.WOULD_BLOCK then a connection is in progress
# and the stream returned in the client argument is valid. In that case the Stream.poll
# function must be used with the PollFlag.CONNECT flag to poll for the completion
# of the connection.
#
# Note that for connectionless protocols such as UDP, the stream_connect will return
# successfully even without a "connection". The stream_connect in this case merely
# establishes the address to which datagrams will be sent.
#
client = Stream()
try:
    is_connected = client.connect(uri, nonblocking, send_buffer_size, receive_buffer_size)
    if not is_connected: # connection in progress

        #
        # Make the timeout a relative timeout of 10 seconds. Note that the stream API supports
        # absolute timeouts as well using the same structure, but setting the is_absolute
        # flag. Use the functions in quanser_time.h for getting the current absolute time
        # and manipulating t_timeout structures (e.g. adding, subtracting, comparing, etc).
        #
        timeout = Timeout(10)

        #
        # Wait for the connection to complete by invoking Stream.poll with the
        # PollFlag.CONNECT flag. Note that Stream.poll can be used with multiple
        # flags at the same time (as a bitmask) and returns a bitmask representing
        # the conditions that were satisfied. However, in this case there is only one
        # flag so bitwise operations are not required.
        #
        # If the timeout expires before the requested event occurs then Stream.poll
        # returns zero. In this case, we raise an exception with error code
        # -ErrorCode.TIMED_OUT so that we let the normal error handling process the
        # error. The Stream.poll function does not return -ErrorCode.TIMED_OUT 
        # directly because timing out is not an error condition and Stream.poll
        # is used in many other instances where a zero return value in the case
        # of a timeout is easier to integrate into code.
        #
        result = client.poll(timeout, PollFlag.CONNECT)
        if result == 0: # timed out waiting to connect
            raise StreamError(-ErrorCode.TIMED_OUT)

    try:
        count = 0
        iterations = 0
        amplitude = 5.0
        time_scale = 0.1
        double_buf = arr.array("d", [0.0])
        no_data_received = True
        do_send = True

        # The client is now connected
        print("Connected to URI '%s'...\n" % uri)

        # The loop for sending/receving data to/from the server
        while not stop:

            #
            # "Send" a double value to the server. The Stream.send_double function stores the double value
            # in the stream's send buffer, but does not necessarily transmit the contents of the send buffer
            # to the underlying communication channel. It only transmits the data if the send buffer becomes
            # full or the stream is flushed. This paradigm is used to maximize throughput by allowing data
            # to be coalesced into fewer packets in packet-based protocols like TCP/IP and UDP/IP.
            #
            # The Stream.send_double call may return -ErrorCode.WOULD_BLOCK, indicating that the data could not
            # be sent at this time without blocking. The Stream.poll function could be used with the
            # PollFlag.SEND flag to ensure that at least one byte of space is available in the stream's
            # send buffer (although Stream.send_double could still return -ErrorCode.WOULD_BLOCK attempting to
            # write an 8-byte double value). To ensure the data is sent, do a loop in which Stream.poll is
            # called first (with a timeout or NULL to block indefinitely) followed by the Stream.send_double.
            # Note that Stream.send_double NEVER writes part of the double into the stream's send buffer.
            # It always treats the complete double as a single atomic unit. In other words, it writes the
            # entire double into the stream buffer or none of it.
            #
            # For example, to block for a certain timeout period (relative_timeout) attempting to send
            # the data, use the following code:
            #
            #    relative_timeout = Timeout(30) # 30 seonds
            #    absolute_timeout = relative_timeout.get_absolute()
            #
            #    while True:
            #        result = client.poll(absolute_timeout, PollFlag.SEND)
            #        if (result == PollFlag.SEND):
            #            result = client.send_double(value)
            #        elif (result == 0): # timed out
            #            raise StreamError(-ErrorCode.TIMED_OUT)  # issue an error since send timed out
            #        if (result != -ErrorCode.WOULD_BLOCK):
            #            break
            #
            # Note that the timeout is converted to absolute so that the loop exits within the original
            # timeout regardless how many times the loop actually executes. The loop will only execute
            # more than once if the stream's send buffer has bytes free but not enough for a complete
            # double value and the data cannot be transmitted immediately via the underlying communication
            # channel.
            #
            # To make non-blocking I/O act like blocking I/O then specify a NULL timeout to the Stream.poll,
            # which indicates an infinite timeout.
            #
            # A similar loop can be done for the Stream.flush using Stream.poll with the PollFlag.FLUSH
            # flag. If data is being flushed after every send operation then the above send loop is unnecessary
            # if the size of the stream send buffer specified in Stream.connect is large enough to hold all
            # the data because in that case it is known in advance that the Stream.send_double will never
            # block. Flushing the stream transmits the data in the stream's send buffer to the underlying
            # communication channel and removes it from the stream's send buffer.
            #
            # For packet-based protocols like TCP/IP the loop may not be necessary either if the data is
            # being flushed every time and is known to be small enough to fit within a single packet. For
            # example, with TCP/IP it is possible to send 1460 bytes in every packet. Hence, for our
            # double, which is only eight bytes, the code could look like the following:
            #
            #     client.send_double(value)
            #     result = client.poll(timeout, PollFlag.FLUSH) # underlying channel has space
            #     if (result == PollFlag.FLUSH):
            #         client.flush() # all data will be sent since it fits in one packet
            #     elif (result == 0): # timed-out. Peer not reading data?
            #         raise StreamError(-PollFlag.TIMED_OUT)
            #
            # A sawtooth waveform is sent to the server. Note that the stream's send buffer was set to the
            # size of a single double in the Stream.connect call to prevent a large collection of data points
            # from accumulating in the send buffer if the Stream.flush kept returning -ErrorCode.WOULD_BLOCK. In
            # other applications such accumulation may be desirable. Note that the Stream.flush sends as much
            # data from the stream's send buffer as it can, so all data in the stream's send buffer will be
            # transmitted eventually. The Stream.flush will return -ErrorCode.WOULD_BLOCK even if it manages to
            # send some of the data. It only returns zero if all the data is flushed and the stream's send
            # buffer is now empty. Also note that Stream.send_double itself will attempt to flush the stream
            # if there is no more space in the stream's send buffer for data.
            #
            # For protocols like UDP in which packets are not acknowledged, this example will keep firing
            # off packets whether the server exists or not, because it does not know if the packets are
            # received. Doing so means that the server may be started after the client, but this behaviour
            # may not be desirable depending on the application. Once the server has responded, we set the
            # no_data_received flag to false so that data is no longer sent indiscriminately, but waits for
            # a data point to be received before sending each value. The do_send flag is used to keep track
            # of each time a data point is received so that only one value is sent for each value received.
            # Note that multiple zero values may be transmitted for any protocol before the first value is
            # received back from the server because of the time required for the server to respond to the
            # first packet. Remember that because non-blocking I/O is being used in this example none of
            # the functions below actually block - all of them return quickly. This fact is readily apparent
            # from the iterations count printed when the example exits. Note that the iteration count will
            # be extremely large if the print issuing the last value received is commented out.
            #
            # For UDP, the server requires that a packet be received from a client before it can respond
            # because it is a connectionless protocol and otherwise would not know to whom data should be
            # sent. See one of the server examples for details.
            #
            if (do_send or no_data_received):
                result = client.send_double(math.fmod(time_scale * count, amplitude))
                if (result > 0):
                    #
                    # To ensure that the data is transmitting immediately, in order to minimize latency at the
                    # possible expense of throughput, flush the stream, as done in this example below. Note
                    # that Stream.flush may return -ErrorCode.WOULD_BLOCK. See the comment above on Stream.send_double
                    # for possible ways of handling this return value. In this case, we ignore the error and
                    # allow a new value to be received if one is available. However, since the Stream.send_double
                    # succeeded above, the value is in the stream's send buffer and will be transmitted the next
                    # time data is flushed to the underlying communication channel.
                    #
                    result = client.flush()
                    if (result == 0): # all the data was flushed
                        if (not no_data_received):
                            count += 1 # count number of doubles transmitted after the server has responded

                        do_send = False # don't send again until next value received from server
                    elif (result < 0 and result != -ErrorCode.WOULD_BLOCK):
                        break
                elif (result != -ErrorCode.WOULD_BLOCK):
                    break

            #
            # Attempt to receive a double value from the server. If the result is zero then the server closed the
            # connection gracefully and this end of the connection should also be closed. If the result is negative
            # then an exception is raised the connection should be closed as well.
            #
            # Note that for the UDP protocol the Stream.connect does not explicitly bind the UDP socket to the port,
            # as the server does, because otherwise the client and server could not be run on the same machine. Hence,
            # the stream implicitly binds the UDP socket to the port when the first Stream.send operation is performed.
            # Thus, a Stream.send should always be executed prior to the first Stream.receive for the UDP
            # protocol. If a Stream.receive is attempted first, using UDP, then the -ErrorCode.CONNECTION_NOT_BOUND
            # error will be returned by Stream.receive. The error is not fatal and simply indicates that a send
            # operation must be performed before data can be received. This restriction only applies to
            # connectionless protocols like UDP.
            #
            # In this example the Stream.send is done first so the connection will be bound to the UDP port already,
            # since the first Stream.send will not block. Note that it is actually the Stream.flush after the Stream.send
            # that binds the UDP port because the Stream.send may not access the underlying communication channel if
            # the stream's send buffer is not full.
            #
            result = client.receive_doubles(double_buf, len(double_buf));
            if (result > 0):
                #
                # Print out the value received. Also slows down the loop a bit so it doesn't run ridiculously fast.
                #
                print("Value: %6.3f" % double_buf[0], end="\r")

                # Flag that we have received data, which indicates the server has responded
                no_data_received = False

                # Flag that a new data point should be sent
                do_send = True
            elif (result != -ErrorCode.WOULD_BLOCK): # then a genuine error occurred or the connection was closed by the peer
                break

            #############################################################################################
            # Do other processing here. This example allows 100% of the CPU time to be used by this loop
            # (since the I/O never blocks) so that the iterations count shows how often the loop is run.
            #############################################################################################/

            # Count the number of iterations (will be much higher than number of data items transmitted)
            iterations += 1

    except StreamError as ex:
        # A communications error occurred. Print the appropriate message.
        print("Error communicating on URI '%s'. %s" % (uri, ex.get_error_message()))

    finally:
        # The connection has been closed at the server end or an error has occurred. Close the client stream.
        client.close()
        print("\nConnection closed. Items processed: %lu. Number of iterations: %lu" % (count, iterations))

except StreamError as ex:
    #
    # If the Stream.connect function encountered an error, the client cannot connect
    # to the server. Print the message corresponding to the error that occured.
    #
    print("Unable to connect to URI '%s'. %s" % (uri, ex.get_error_message()))

input("Press Enter to exit")
exit(0)
