######################################################################
#
# stream_client_blocking_example.py - Python file
#
# Quanser Stream Python API Client Blocking I/O Example.
# Communication Loopback Demo.
# 
# This example uses the Quanser Stream Python functions to connect 
# to Simulink server models and send/receive data to/from them. This
# application uses blocking I/O for sending/receiving data.
#
# This example demonstrates the use of the following functions:
#    Stream.connect
#    Stream.receive_doubles
#    Stream.send_double
#    Stream.flush
#    Stream.close
#
# Copyright (C) 2023 Quanser Inc.
#
######################################################################

from quanser.communications import Stream, StreamError
import signal
import array as arr

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

print("Quanser Stream Client Blocking I/O Example\n");
print("Press Ctrl+C to stop\n");
print("Connecting to URI '%s'..." % uri);

#
# This function attempts to connect to the server using the specified URI. For UDP,
# which is a connectionless protocol, this call will return immediately.
#
client = Stream()
try:
    is_connected = client.connect(uri, nonblocking, send_buffer_size, receive_buffer_size)
    if is_connected: # now connected

        amplitude = 5.0;
        time_scale = 0.1;
        count = 0;
        double_buf = arr.array("d", [0.0])

        print("Connected to URI '%s'...\n" % uri)

        # The loop for sending/receving data to/from the server
        try:
            while not stop:
                #
                # "Send" a double value to the server. The Stream.send_double function stores the double value
                # in the stream's send buffer, but does not necessarily transmit the contents of the send buffer
                # to the underlying communication channel. It only transmits the data if the send buffer becomes
                # full or the stream is flushed. This paradigm is used to maximize throughput by allowing data 
                # to be coalesced into fewer packets in packet-based protocols like TCP/IP and UDP/IP.
                #
                # A sawtooth waveform is sent to the server.
                #
                client.send_double((time_scale * count) % amplitude)
            
                # 
                # To ensure that the data is transmitting immediately, in order to minimize latency at the
                # possible expense of throughput, flush the stream, as done in this example below.
                #
                client.flush()
            
                # 
                # Attempt to receive a double value from the server. If the result is zero then the server closed the
                # connection gracefully and this end of the connection should also be closed. If an exception is thrown
                # then an error occurred and the connection should be closed as well.
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
                result = client.receive_doubles(double_buf, 1)
                if result <= 0:
                    break
            
                print("Value: %6.3lf" % double_buf[0], end='\r')

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
    # If the Stream.connect function encountered an error, the client cannot connect
    # to the server. Print the message corresponding to the error that occured.
    #
    print("Unable to connect to URI '%s'. %s" % (uri, ex.get_error_message()))
    
input("Press Enter to exit")
exit(0)
