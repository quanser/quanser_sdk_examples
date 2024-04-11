CFLAGS += -I/usr/include/quanser
LIBS   += -lhil -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

hil_read_encoder_performance: hil_read_encoder_performance.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

hil_read_encoder_performance.o: hil_read_encoder_performance.c hil_read_encoder_performance.h
