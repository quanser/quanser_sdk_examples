CFLAGS  += -I/opt/quanser/hil_sdk/include
LDFLAGS += -L/opt/quanser/hil_sdk/lib
LIBS    += -lhil -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

hil_get_string_property_example: hil_get_string_property_example.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

hil_get_string_property_example.o: hil_get_string_property_example.c hil_get_string_property_example.h
