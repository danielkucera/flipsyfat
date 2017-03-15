include ../common.mak

OBJECTS = main.o $(COMMON)/sdemu.o $(COMMON)/fat.o $(COMMON)/isr.o
APP = simple

all: $(APP).bin

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	chmod -x $@

$(APP).elf: $(OBJECTS) libs

%.elf:
	$(LD) $(LDFLAGS) \
		-T $(MISOC_DIRECTORY)/software/libbase/linker-sdram.ld \
		-N -o $@ \
		$(MSCDIR)/software/libbase/crt0-$(CPU).o \
		$(OBJECTS) \
		-L$(MSCDIR)/software/libbase \
		-L$(MSCDIR)/software/libcompiler-rt \
		-L$(LIBGCC_DIR) \
		-lcompiler-rt -lbase-nofloat -lgcc
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

load: $(APP).bin
	echo Connecting to bootloader...
	$(MISOC_DIRECTORY)/tools/flterm.py --kernel $(APP).bin $(SERIAL)

term:
	miniterm.py --raw $(SERIAL) 115200

clean:
	$(RM) $(OBJECTS) $(APP).elf $(APP).bin
	$(RM) .*~ *~

.PHONY: all main.o clean libs load term
