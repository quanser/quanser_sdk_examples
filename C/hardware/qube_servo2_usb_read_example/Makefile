CFLAGS += -I/usr/include/quanser
LIBS   += -lhil -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

qube_servo2_usb_read_example: qube_servo2_usb_read_example.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

qube_servo2_usb_read_example.o: qube_servo2_usb_read_example.c qube_servo2_usb_read_example.h
