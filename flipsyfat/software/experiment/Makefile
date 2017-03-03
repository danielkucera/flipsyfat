include $(MISOC_DIRECTORY)/software/common.mak

OBJECTS=isr.o main.o sdemu.o

all: experiment.bin

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	chmod -x $@

experiment.elf: $(OBJECTS) libs

%.elf:
	$(LD) $(LDFLAGS) \
		-T $(MISOC_DIRECTORY)/software/libbase/linker-sdram.ld \
		-N -o $@ \
		$(MSCDIR)/software/libbase/crt0-$(CPU).o \
		$(OBJECTS) \
		-L$(MSCDIR)/software/libbase \
		-L$(MSCDIR)/software/libcompiler-rt \
		-lcompiler-rt -lbase-nofloat
	chmod -x $@

main.o: main.c
	$(compile)

%.o: %.c
	$(compile)

%.o: %.S
	$(assemble)

libs:
	$(MAKE) -C $(MSCDIR)/software/libcompiler-rt
	$(MAKE) -C $(MSCDIR)/software/libbase

load: experiment.bin
	echo Connecting to bootloader...
	$(MISOC_DIRECTORY)/tools/flterm.py --kernel experiment.bin $(SERIAL)

clean:
	$(RM) $(OBJECTS) experiment.elf experiment.bin
	$(RM) .*~ *~

.PHONY: all main.o clean libs load
