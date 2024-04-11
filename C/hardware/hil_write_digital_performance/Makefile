CFLAGS += -I/usr/include/quanser
LIBS   += -lhil -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

hil_write_digital_performance: hil_write_digital_performance.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

hil_write_digital_performance.o: hil_write_digital_performance.c hil_write_digital_performance.h
