CFLAGS += -I/usr/include/quanser
LIBS   += -lhil -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

hil_write_analog_buffer_example: hil_write_analog_buffer_example.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

hil_write_analog_buffer_example.o: hil_write_analog_buffer_example.c hil_write_analog_buffer_example.h
