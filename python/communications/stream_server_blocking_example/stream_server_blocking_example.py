######################################################################
#
# stream_server_blocking_example.py - Python file
#
# Quanser Stream Python API Server Blocking I/O Example.
# Communication Loopback Demo.
# 
# This example uses the Quanser Stream Python functions to accept connections 
# from Simulink client models and send/receive data to/from them. This
# application uses blocking I/O for sending/receiving data.
#
# This example demonstrates the use of the following functions:
#    Stream.listen
#    Stream.accept
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

print("Quanser Stream Server Blocking I/O Example\n")
print("Press Ctrl+C to stop\n")

#
# This function establishes a server stream which listens on the given URI.
#
server = Stream()
try:
    server.listen(uri, nonblocking)

    double_buf = arr.array("d", [0.0])

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

                try:
                    while not stop:
                        # 
                        # Attempt to receive a double value from the client. If the result is zero then the client closed the
                        # connection gracefully and this end of the connection should also be closed. If an exception is thrown
                        # then the connection should be closed as well. Note that UDP is a connectionless  protocol so normally 
                        # the server would not be able to detect if the client closed its "connection". However, the stream API 
                        # sends a zero-length datagram by default when a UDP "connection" is closed so that the server can detect
                        # it (assuming the packet isn't lost enroute). Transmission of a zero-length datagram on closure can be 
                        # disabled via the UDP options in the URI. See the stream API documentation on the UDP protocol URI for
                        # details.
                        #
                        result = client.receive_doubles(double_buf, 1);
                        if result <= 0:
                            break
                    
                        print("Value: %6.3lf" % double_buf[0], end="\r")
                    
                        # Count the number of values received
                        count += 1
                    
                        #
                        # "Send" a double value to the client. The stream_send_double function stores the double value
                        # in the stream's send buffer, but does not necessarily transmit the contents of the send buffer
                        # to the underlying communication channel. It only transmits the data if the send buffer becomes
                        # full or the stream is flushed. This paradigm is used to maximize throughput by allowing data 
                        # to be coalesced into fewer packets in packet-based protocols like TCP/IP and UDP/IP.
                        #
                        # This particular example simply echoes the received value back to the client who connected.
                        #
                        client.send_double(double_buf[0])

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
