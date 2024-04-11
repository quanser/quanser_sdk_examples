CFLAGS += -I/usr/include/quanser
LIBS   += -lhil -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

analog_loopback_example: analog_loopback_example.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

analog_loopback_example.o: analog_loopback_example.c analog_loopback_example.h
