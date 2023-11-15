CFLAGS  += -I/opt/quanser/hil_sdk/include
LDFLAGS += -L/opt/quanser/hil_sdk/lib
LIBS    += -lhil -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

qube_servo2_usb_control_example: qube_servo2_usb_control_example.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

qube_servo2_usb_control_example.o: qube_servo2_usb_control_example.c qube_servo2_usb_control_example.h
