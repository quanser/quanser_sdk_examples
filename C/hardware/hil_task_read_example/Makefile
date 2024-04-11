CFLAGS += -I/usr/include/quanser
LIBS   += -lhil -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

hil_task_read_example: hil_task_read_example.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

hil_task_read_example.o: hil_task_read_example.c hil_task_read_example.h
