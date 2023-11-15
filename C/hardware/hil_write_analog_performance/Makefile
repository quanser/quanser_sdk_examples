CFLAGS  += -I/opt/quanser/hil_sdk/include
LDFLAGS += -L/opt/quanser/hil_sdk/lib
LIBS    += -lhil -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

hil_write_analog_performance: hil_write_analog_performance.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

hil_write_analog_performance.o: hil_write_analog_performance.c hil_write_analog_performance.h
