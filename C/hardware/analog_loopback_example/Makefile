CFLAGS  += -I/opt/quanser/hil_sdk/include
LDFLAGS += -L/opt/quanser/hil_sdk/lib
LIBS    += -lhil -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

analog_loopback_example: analog_loopback_example.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

analog_loopback_example.o: analog_loopback_example.c analog_loopback_example.h
