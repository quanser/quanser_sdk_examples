CFLAGS += -I/usr/include/quanser
LIBS   += -lhil -lquanser_runtime -lquanser_common -lrt -lpthread -ldl -lm -lc

haptic_wand_example: haptic_wand_example.o
	$(CC) $(LDFLAGS) $< -o $@ $(LIBS)

haptic_wand_example.o: haptic_wand_example.c haptic_wand_example.h
