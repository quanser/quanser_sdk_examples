CFLAGS  += -I/opt/quanser/hil_sdk/include
LDFLAGS += -L/opt/quanser/hil_sdk/lib
LIBS    += -lhil -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

hil_write_digital_example: hil_write_digital_example.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

hil_write_digital_example.o: hil_write_digital_example.c hil_write_digital_example.h
