######################################################################
#
# stream_server_mixed_types_blocking_example.py - Python file
#
# Quanser Stream Python API Server Mixed Types Blocking I/O Example.
# Communication Loopback Demo.
#
# This example uses the Quanser Stream Python functions to accept connections
# from Simulink client models and send/receive data to/from them. This
# application uses blocking I/O for sending/receiving data.
#
# This example demonstrates the use of the following functions:
#    Stream.listen
#    Stream.accept
#    Stream.peek_begin
#    Stream.peek_byte_array
#    Stream.peek_short_array
#    Stream.peek_end
#    Stream.poke_begin
#    Stream.poke_double
#    Stream.poke_byte
#    Stream.poke_single_array
#    Stream.poke_end
#    Stream.flush
#    Stream.close
#
# Copyright (C) 2023 Quanser Inc.
#
######################################################################

from quanser.communications import Stream, StreamError
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

nonblocking         = False
send_buffer_size    = 8000
receive_buffer_size = 8000

amplitude  = 2.5
time_scale = 0.01
byte_buf   = arr.array("b", [0] * 3)
word_buf   = arr.array("h", [0])
float_buf  = arr.array("f", [0.0] * 3)
double_buf = arr.array("d", [0.0])

# Register a Ctrl+C handler
signal.signal(signal.SIGINT, signal_handler)

print("Quanser Stream Server Mixed Types Blocking I/O Example\n");
print("Press Ctrl+C to stop (when client connected)\n");

#
# This function establishes a server stream which listens on the given URI.
#
server = Stream()
try:
    server.listen(uri, nonblocking)

    print("Listening on URI '%s'...\n" % uri)

    try:
        #
        # Continue to accept client connections, one at a time, so that the server
        # does not stop running after the first client closes their connection.
        #
        while not stop:
            print("Waiting for a new connection from a client...")

            #
            # This function accepts new connections from clients using the server
            # stream created by Stream.listen. Upon successful operation, it creates
            # a client stream to be used for communications. For connectionless protocols
            # like UDP it returns immediately and the peer for communications is determined
            # by the first client to send data to the server (see stream API documentation on
            # the UDP protocol URI for options with respect to subsequent clients).
            #
            # Note that Stream.poll may be used with the PollFlag.ACCEPT flag and a
            # timeout to poll the stream for any pending client connections. Doing so
            # would allow the Ctrl+C to be detected on Windows without having a client
            # connect.
            #
            # Also, for connectionless protocols like UDP the Stream.accept call will
            # return immediately as if a client had connected. The client to whom
            # communications should be established is then determined by the first client
            # to send data to the server. Thus, a stream receive operation should always
            # be done first for the UDP protocol. If a stream send is attempted prior to
            # a stream receive then an exception with error code -ErrorCode.NO_DESIGNATED_PEER 
            # will raised, indicating that the server does not know to whom data should be
            # sent because no data has been received from a client. This error is not fatal
            # and simply indicates that a stream receive must be done to determine the peer
            # before data can be sent. This restriction only applies to connectionless 
            # protocols like UDP.
            #
            # How subsequent clients to send data to the server are handled depends on
            # the options specified in the UDP URI.
            #
            client = server.accept(send_buffer_size, receive_buffer_size)
            if client is not None:

                count = 0

                print("Accepted a connection from a client.")
                print("Sending and receiving data.\n")

                try:
                    while not stop:

                        #
                        # Attempt to receive mixed data types from the client. If the result is zero then the client closed the
                        # connection gracefully and this end of the connection should also be closed. If an exception is thrown
                        # then the connection should be closed as well. Note that UDP is a connectionless  protocol so normally 
                        # the server would not be able to detect if the client closed its "connection". However, the stream API 
                        # sends a zero-length datagram by default when a UDP "connection" is closed so that the server can detect
                        # it (assuming the packet isn't lost enroute). Transmission of a zero-length datagram on closure can be 
                        # disabled via the UDP options in the URI. See the stream API documentation on the UDP protocol URI for
                        # details.
                        #
                        # The Stream.peek_begin function begins the process of receiving mixed data types atomically from a
                        # communication channel. In this case, the Stream.peek_byte_array and Stream.peek_short_array
                        # functions are then used to receive the data from the stream. If any of the stream peek functions
                        # fails then none of the data is removed from the stream's receive buffer. The buffer must be large
                        # enough to hold the data "peeked" between the Stream.peek_begin and Stream.peek_end call. The
                        # Stream.peek_end call completes the process by removing the "peeked" data from the stream's receive
                        # buffer is the previous operations were successful (i.e. if the result passed as an argument is 1).
                        #
                        # If the result passed to Stream.peek_end is less than one then it simply returns the value of result
                        # and the data remains in the stream's receive buffer. Otherwise, it removes the data from the stream's
                        # receive buffer and returns 1.
                        #
                        # Note that for the UDP protocol the Stream.connect does not explicitly bind the UDP socket to the port,
                        # as the server does, because otherwise the client and server could not be run on the same machine. Hence,
                        # the stream implicitly binds the UDP socket to the port when the first stream send operation is performed.
                        # Thus, a stream send should always be executed prior to the first stream receive for the UDP
                        # protocol. If a stream receive is attempted first, using UDP, then an exception is thrown with error
                        # code ``-ErrorCode.CONNECTION_NOT_BOUND``. The error is not fatal and simply indicates that a send
                        # operation must be performed before data can be received. This restriction only applies to
                        # connectionless protocols like UDP.
                        #
                        # The number of bytes to prefetch passed to Stream.peek_begin is zero. It could be:
                        #    3 * sizeof(bytes[0]) + sizeof(t_int16)
                        # in order to ensure that the required number of bytes is prefetched, even for protocols like I2C
                        # that do not support read-ahead on the stream. However, protocols support read-ahead by default
                        # so most protocols will attempt to prefetch as much data as possible any time a peek or receive
                        # operation is performed. Hence, a prefetch of zero is an efficient and acceptable value for
                        # most protocols. Using a value of zero for protocols which do not support read-ahead, like I2C,
                        # will still work but will be less efficient than specifying the number of bytes to prefetch.
                        #
                        result, peek_state = client.peek_begin(0)
                        if result > 0:
                            result = client.peek_byte_array(peek_state, byte_buf, len(byte_buf))
                            if result > 0:
                                result = client.peek_short_array(peek_state, word_buf, len(word_buf))

                            result = client.peek_end(peek_state, result)

                        if result <= 0:
                            break

                        print("Values: [%4d %4d %4d], %5u" % (byte_buf[0], byte_buf[1], byte_buf[2], word_buf[0]), end="\r")

                        # Count the number of values received
                        count += 1

                        #
                        # "Send" multiple data types consisting of a double, byte and a single array to the client.
                        # The Stream.poke_begin function begins the process of writing mixed data types atomically to the
                        # stream's send buffer. In this case, the Stream.poke_double, Stream.poke_byte and
                        # Stream.poke_single_array functions are then used to write the actual data to the stream's send
                        # buffer. Finally, the Stream.poke_end function is called to complete the process. If any one of
                        # the stream poke functions fails then nothing is written to the stream's send buffer. The buffer
                        # must be large enough to hold the data "poked" between the Stream.poke_begin and Stream.poke_end
                        # call. The Stream.poke_end call does not necessarily transmit the contents of the send buffer to
                        # the underlying communication channel. It only transmits the data if the send buffer becomes full
                        # or the stream is flushed. This paradigm is used to maximize throughput by allowing data to be
                        # coalesced into fewer packets in packet-based protocols like TCP/IP and UDP/IP.
                        #
                        # If the result passed to Stream.poke_end is less than one then it simply returns the value of result
                        # and the data is removed from the stream's send buffer and not transmitted. Otherwise, it adds the
                        # data to the stream's send buffer and returns 1.
                        #
                        # A sawtooth waveform is written for the double value, a pulse train for the byte, and the single
                        # array consists of three sinusoidal waveforms of amplitude 1, 2 and 3 respectively.
                        #
                        result, poke_state = client.poke_begin()
                        if result >= 0:
                            result = client.poke_double(poke_state, math.fmod(count * time_scale, amplitude))
                            if result > 0:
                                result = client.poke_byte(poke_state, math.fmod(count * time_scale, 1) > 0.5)
                                if result > 0:
                                    value = math.sin(2 * math.pi * time_scale * count)
                                    float_buf[0] = value
                                    float_buf[1] = 2 * value
                                    float_buf[2] = 3 * value

                                    result = client.poke_single_array(poke_state, float_buf, len(float_buf))

                            result = client.poke_end(poke_state, result)

                        #
                        # To ensure that the data is transmitting immediately, in order to minimize latency at the
                        # possible expense of throughput, flush the stream, as done in this example below.
                        #
                        client.flush()

                except StreamError as ex:
                    # A communications error occurred.Print the appropriate message.
                    print("Error communicating with the client. %s" % ex.get_error_message())

                finally:
                    client.close()
                    print("\nConnection closed. Number of items: %lu" % count)

    except StreamError as ex:
        # A communications error occurred. Print the appropriate message.
        print("Unable to accept connections on URI '%s'. %s" % (uri, ex.get_error_message()))

    finally:
        # An error has occurred. Close the server stream.
        server.close()

except StreamError as ex:
    #
    # If the Stream.listen function encountered an error, the server cannot be
    # started. Print the message corresponding to the error that occured.
    #
    print("Unable to listen to URI '%s'. %s" % (uri, ex.get_error_message()))
    
input("Press Enter to exit")
exit(0)
