CFLAGS  += -I/opt/quanser/hil_sdk/include
LDFLAGS += -L/opt/quanser/hil_sdk/lib
LIBS    += -lhil -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

hil_read_encoder_example: hil_read_encoder_example.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

hil_read_encoder_example.o: hil_read_encoder_example.c hil_read_encoder_example.h
