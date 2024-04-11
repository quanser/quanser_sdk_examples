CFLAGS += -I/usr/include/quanser
LIBS   += -lquanser_devices -lquanser_communications -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

aaaf5050_mc_k12_led_example: aaaf5050_mc_k12_led_example.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

aaaf5050_mc_k12_led_example.o: aaaf5050_mc_k12_led_example.c pch.h

clean:
	rm -f *.o aaaf5050_mc_k12_led_example

.PHONY: clean
