CFLAGS += -I/usr/include/quanser
LIBS   += -lquanser_devices -lquanser_communications -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

ws0010_display_image_example: ws0010_display_image_example.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

ws0010_display_image_example.o: ws0010_display_image_example.c pch.h

clean:
	rm -f *.o ws0010_display_image_example

.PHONY: clean
