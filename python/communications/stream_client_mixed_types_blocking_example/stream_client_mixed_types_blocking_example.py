######################################################################
#
# stream_client_mixed_types_blocking_example.py - Python file
#
# Quanser Stream Python API Client Mixed Types Blocking I/O Example.
# Communication Loopback Demo.
# 
# This example uses the Quanser Stream Python functions to connect 
# to Simulink server models and send/receive data to/from them. This
# application uses blocking I/O for sending/receiving data.
#
# This example demonstrates the use of the following functions:
#    Stream.connect
#    Stream.peek_begin
#    Stream.peek_double_array
#    Stream.peek_byte_array
#    Stream.peek_single_array
#    Stream.peek_end
#    Stream.poke_begin
#    Stream.poke_byte_array
#    Stream.poke_short
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

# Register a Ctrl+C handler
signal.signal(signal.SIGINT, signal_handler)

print("Quanser Stream Client Mixed Types Blocking I/O Example\n");
print("Press Ctrl+C to stop\n");
print("Connecting to URI '%s'..." % uri);

#
# This function attempts to connect to the server using the specified URI.For UDP,
# which is a connectionless protocol, this call will return immediately.
#
client = Stream()
try:
    is_connected = client.connect(uri, nonblocking, send_buffer_size, receive_buffer_size)
    if is_connected : # now connected
        time_scale = 0.01
        count = 0
        bytes = arr.array("b", [0, 0, 0])
        float_buf = arr.array("f", [0.0, 0.0, 0.0])
        double_buf = arr.array("d", [0.0])

        print("Connected to URI '%s'...\n" % uri)

        # The loop for sending/receving data to/from the server
        try:
            while not stop:
                #
                # "Send" multiple data types consisting of a 3-element byte array and a short to the server. 
                # The Stream.poke_begin function begins the process of writing mixed data types atomically to the 
                # stream's send buffer. In this case, the Stream.poke_byte_array and Stream.poke_short functions 
                # are then used to write the actual data to the stream's send buffer. Finally, the Stream.poke_end 
                # function is called to complete the process. If any one of the stream poke functions fails then 
                # nothing is written to the stream's send buffer. The buffer must be large enough to hold the data
                # "poked" between the Stream.poke_begin and Stream.poke_end call. The Stream.poke_end call
                # does not necessarily transmit the contents of the send buffer to the underlying communication 
                # channel. It only transmits the data if the send buffer becomes full or the stream is flushed. 
                # This paradigm is used to maximize throughput by allowing data to be coalesced into fewer packets 
                # in packet-based protocols like TCP/IP and UDP/IP.
                #
                # If the result passed to Stream.poke_end is less than one then it simply returns the value of result
                # and the data is removed from the stream's send buffer and not transmitted. Otherwise, it adds the 
                # data to the stream's send buffer and returns 1.
                #
                # The byte (uint8) array consists of three sinusoidal waveforms of amplitude 40, 80 and 120
                # respectively. A sawtooth waveform of amplitude 65535 is written as the short (uint16) value. 
                #
                result, poke_state = client.poke_begin()
                if result == 0:
                    value = 40 * math.sin(2 * math.pi * time_scale * count)
                    bytes[0] = math.trunc(value)
                    bytes[1] = math.trunc(2 * value)
                    bytes[2] = math.trunc(3 * value)

                    result = client.poke_byte_array(poke_state, bytes, 3)
                    if result > 0:
                        result = client.poke_short(poke_state, math.trunc(math.fmod(1000 * count, 32768)))

                    result = client.poke_end(poke_state, result)

                # 
                # To ensure that the data is transmitting immediately, in order to minimize latency at the
                # possible expense of throughput, flush the stream, as done in this example below.
                #
                client.flush()
            
                # 
                # Attempt to receive mixed data types from the server. If the result is zero then the server closed the
                # connection gracefully and this end of the connection should also be closed. If an exception occurs
                # then the connection should be closed as well.
                #
                # The Stream.peek_begin function begins the process of receiving mixed data types atomically from a
                # communication channel. In this case, the Stream.peek_double_array, Stream.peek_byte_array and 
                # Stream.peek_single_array functions are then used to receive the data from the stream. If any of the
                # stream peek functions fails then none of the data is removed from the stream's receive buffer. The 
                # buffer must be large  enough to hold the data "peeked" between the Stream.peek_begin and Stream.peek_end
                # call. The Stream.peek_end call completes the process by removing the "peeked" data from the stream's receive
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
                # code -ErrorCode.CONNECTION_NOT_BOUND by the stream receive. The error is not fatal and simply indicates 
                # that a send operation must be performed before data can be received. This restriction only applies to
                # connectionless protocols like UDP.
                #
                # The number of bytes to prefetch passed to Stream.peek_begin is zero. It could be:
                #    sizeof(value) + sizeof(bytes[0]) + 3 * sizeof(float_values[0])
                # in order to ensure that the required number of bytes is prefetched, even for protocols like I2C
                # that do not support read - ahead on the stream.However, protocols support read - ahead by default
                # so most protocols will attempt to prefetch as much data as possible any time a peek or receive
                # operation is performed.Hence, a prefetch of zero is an efficientand acceptable value for
                # most protocols.Using a value of zero for protocols which do not support read - ahead, like I2C,
                # will still work but will be less efficient than specifying the number of bytes to prefetch.
                #
                result, peek_state = client.peek_begin(0)
                if result > 0:
                    result = client.peek_double_array(peek_state, double_buf, 1)
                    if result > 0:
                        result = client.peek_byte_array(peek_state, bytes, 1)
                    if result > 0:
                        result = client.peek_single_array(peek_state, float_buf, 3)

                    result = client.peek_end(peek_state, result)

                if result <= 0:
                    break

                print("Values: %6.3lf, %d, [%6.3f %6.3f %6.3f]" % (double_buf[0], bytes[0], float_buf[0], float_buf[1], float_buf[2]), end="\r")

                # Count the number of values sent and received
                count += 1

        except StreamError as ex:
            # A communications error occurred. Print the appropriate message.
            print("Error communicating on URI '%s'. %s" % (uri, ex.get_error_message()))

        finally:
            # The connection has been closed at the server end or an error has occurred. Close the client stream.
            client.close()
            print("\nConnection closed. Number of items: %lu" % count)
    
except StreamError as ex:
    #
    # If the stream_connect function encountered an error, the client cannot connect
    # to the server. Print the message corresponding to the error that occured.
    #
    print("Unable to connect to URI '%s'. %s" % (uri, ex.get_error_message()))
    
input("Press Enter to exit")
exit(0)
