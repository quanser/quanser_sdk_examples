CFLAGS += -I/usr/include/quanser
LIBS   += -lhil -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

position_control_example: position_control_example.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

position_control_example.o: position_control_example.c position_control_example.h
