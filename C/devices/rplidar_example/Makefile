CFLAGS += -I/usr/include/quanser
LIBS   += -lquanser_devices -lquanser_communications -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

rplidar_example: rplidar_example.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

rplidar_example.o: rplidar_example.c pch.h

clean:
	rm -f *.o rplidar_example

.PHONY: clean
