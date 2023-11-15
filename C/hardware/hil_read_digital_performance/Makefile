CFLAGS  += -I/opt/quanser/hil_sdk/include
LDFLAGS += -L/opt/quanser/hil_sdk/lib
LIBS    += -lhil -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

hil_read_digital_performance: hil_read_digital_performance.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

hil_read_digital_performance.o: hil_read_digital_performance.c hil_read_digital_performance.h
