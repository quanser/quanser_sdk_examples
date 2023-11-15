######################################################################
#
# stream_server_nonblocking_example.py - Python file
#
# Quanser Stream Python API Server Non-Blocking I/O Example.
# Communication Loopback Demo.
# 
# This example uses the Quanser Stream Python functions to accept connections 
# from Simulink client models and send/receive data to/from them. This
# application uses non-blocking I/O for sending/receiving data.
#
# This example demonstrates the use of the following functions:
#    Stream.listen
#    Stream.poll
#    Stream.accept
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

    
print("Quanser Stream Server Non-Blocking I/O Example\n")
print("Press Ctrl+C to stop\n")
    
#
# This function establishes a server stream which listens on the given URI.
#
server = Stream()
try:
    server.listen(uri, nonblocking)

    STATE_RECEIVE = 0
    STATE_SEND = 1
    STATE_FLUSH = 2

    wait_symbols = "|/-\\"
    wait_symbol_index = 0

    double_buf = arr.array("d", [0.0])

    timeout = Timeout(0, 30000000) # 0.030 seconds
    server_iterations = 0

    print("Listening on URI '%s'...\n" % uri)

    try:
        #
        # Continue to accept client connections, one at a time, so that the server
        # does not stop running after the first client closes their connection.
        #
        while not stop:
            # Print waiting message with spinning wait symbol
            print("Waiting for a new connection from a client: %c..." % wait_symbols[wait_symbol_index], end="\r")
            wait_symbol_index = (wait_symbol_index + 1) % len(wait_symbols)

            #
            # Poll the server stream to determine if there are any pending client
            # connections. Wait up to 0.030 seconds and then timeout. Note that the
            # Stream.poll function can poll for more than one condition at the
            # same time, as the last argument is a bitmask. It returns a bitmask
            # indicating which conditions were satisfied. If it returns zero then
            # it timed out before any of the events occurred. If an error occurs
            # it raises an exception.
            #
            # Also, for connectionless protocols like UDP the Stream.poll call will
            # return immediately as if a client had connected. The client to whom
            # communications should be established is then determined by the first client
            # to send data to the server. Thus, a stream receive operation should always
            # be done first for the UDP protocol. If a stream_send is attempted prior to
            # a stream receive then -ErrorCode.NO_DESIGNATED_PEER will be returned, indicating
            # that the server does not know to whom data should be sent because no data
            # has been received from a client. This error is not fatal and simply indicates
            # that a stream receive must be done to determine the peer before data can be
            # sent. This restriction only applies to connectionless protocols like UDP.
            #
            # How subsequent clients to send data to the server are handled depends on
            # the options specified in the UDP URI.
            #
            result = server.poll(timeout, PollFlag.ACCEPT)
            if (result == PollFlag.ACCEPT):  # then connection pending

                state = STATE_RECEIVE
                
                #
                # A client connection is pending. Accept the connection and proceed to commmunicate
                # with the client. See notes above on the Stream.poll invocation for the behaviour
                # of UDP.
                #
                client = server.accept(send_buffer_size, receive_buffer_size)
                if client is not None:
                    print("\nAccepted a connection from a client")
                    print("Sending and receiving data.\n")
                
                    client_iterations = 0
                    count = 0

                    try:
                        while not stop:

                            #
                            # Use a simple state machine to communicate with the client. First receive a value from the client.
                            # Then send that value back to the client and finally flush the stream so that the data is sent
                            # immediately rather than waiting for the stream send buffer to become full first. A state machine
                            # is used because the stream functions could return -ErrorCode.WOULD_BLOCK at any stage, indicating that
                            # it was not possible to complete the operation without blocking. In that case the state does not
                            # change and it attempts to do the operation again the next time around the loop.
                            #
                            if state == STATE_RECEIVE:
                                # 
                                # Attempt to receive a double value from the server. If the result is zero then the server closed the
                                # connection gracefully and this end of the connection should also be closed. If an exception is raised
                                # then an error occurred and the connection should be closed as well. If -ErrorCode.WOULD_BLOCK is
                                # returned it indicates that the double value could not be received without blocking. In this case, 
                                # more attempts to receive the value may be done or other operations may be performed.
                                #
                                # Note that UDP is a connectionless protocol so normally the server would not be able to detect if the
                                # client closed its "connection". However, the stream API sends a zero-length datagram by default when a UDP 
                                # "connection" is closed so that the server can detect it (assuming the packet isn't lost enroute).
                                # Transmission of a zero-length datagram on closure can be disable via the UDP options in the URI.
                                # See the stream API documentation on the UDP protocol URI for details.
                                #
                                # The Stream.receive_double function never reads only some of the bytes of the double from the stream.
                                # It either retrieves an entire double from the stream or nothing. In other words, the double value is
                                # treated as an atomic unit. These semantics make it much easier to receive multibyte data types using
                                # non-blocking I/O. Using the stream peek set of functions, it is even possible to retrieve complex
                                # data types. Also refer to the Stream.set_swap_bytes function to account for byte-order differences
                                # between the client and server.
                                #
                                result = client.receive_doubles(double_buf, len(double_buf))
                                if (result == 0): # then peer closed connection
                                    break
                                elif (result > 0): # then a double was received
                                    print("Value: %6.3lf" % double_buf[0], end="\r")
                            
                                    # Count the number of values received
                                    count += 1
                            
                                    # Flag that the value should be sent back to the client
                                    state = STATE_SEND
                            
                                    # Fall through deliberately so the send is attempted right away
                    
                            if state == STATE_SEND:
                                #
                                # "Send" a double value to the client. The Stream.send_double function stores the double value
                                # in the stream's send buffer, but does not necessarily transmit the contents of the send buffer
                                # to the underlying communication channel. It only transmits the data if the send buffer becomes
                                # full or the stream is flushed. This paradigm is used to maximize throughput by allowing data 
                                # to be coalesced into fewer packets in packet-based protocols like TCP/IP and UDP/IP.
                                #
                                # This particular example simply echoes the received value back to the client who connected.
                                #
                                result = client.send_double(double_buf[0])
                                if (result >= 0): # then the double was placed in the stream send buffer successfully
                                    state = STATE_FLUSH
                            
                                    # Fall through deliberately so the flush is attempted right away

                            if state == STATE_FLUSH:
                                # 
                                # To ensure that the data is transmitting immediately, in order to minimize latency at the
                                # possible expense of throughput, flush the stream, as done in this example below.
                                #
                                result = client.flush()
                                if (result >= 0):
                                    state = STATE_RECEIVE
                                    result = 1 # prevent (result <= 0) test below from causing loop to exit

                            #############################################################################################
                            # Do other processing here. This example allows 100% of the CPU time to be used by this loop
                            # (since the I/O never blocks) so that the iterations count shows how often the loop is run.
                            #############################################################################################/

                            # Count the number of iterations of the loop
                            client_iterations += 1

                    except StreamError as ex:
                        # A communications error occurred.Print the appropriate message.
                        print("\nError communicating with the client. %s" % ex.get_error_message())

                    finally:
                        client.close()
                        print("\nConnection closed. Items processed: %lu. Number of iterations: %lu" % (count, client_iterations))

            server_iterations += 1

    except StreamError as ex:
        # A communications error occurred. Print the appropriate message.
        print("\nUnable to accept connections on URI '%s'. %s" % (uri, ex.get_error_message()))

    finally:
        server.close()
        print("\nServer closed. Number of iterations: %lu" % server_iterations)

except StreamError as ex:
    #
    # If the Stream.listen function encountered an error, the server cannot be
    # started. Print the message corresponding to the error that occured.
    #
    print("\nUnable to listen to URI '%s'. %s" % (uri, ex.get_error_message()))
    
input("\nPress Enter to exit")
exit(0)
