CFLAGS  += -I/opt/quanser/hil_sdk/include
LDFLAGS += -L/opt/quanser/hil_sdk/lib
LIBS    += -lhil -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

hil_task_write_continuously_example: hil_task_write_continuously_example.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

hil_task_write_continuously_example.o: hil_task_write_continuously_example.c hil_task_write_continuously_example.h
