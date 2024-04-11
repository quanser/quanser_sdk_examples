CFLAGS += -I/usr/include/quanser
LIBS   += -lquanser_devices -lquanser_communications -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

ws0010_display_example: ws0010_display_example.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

ws0010_display_example.o: ws0010_display_example.c pch.h

clean:
	rm -f *.o ws0010_display_example

.PHONY: clean
