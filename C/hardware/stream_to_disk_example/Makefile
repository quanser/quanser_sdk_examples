CFLAGS  += -I/opt/quanser/hil_sdk/include
LDFLAGS += -L/opt/quanser/hil_sdk/lib
LIBS    += -lhil -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

stream_to_disk_example: stream_to_disk_example.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

stream_to_disk_example.o: stream_to_disk_example.c stream_to_disk_example.h
