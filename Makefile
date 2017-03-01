PAPILIO_DIR := $(shell pwd)/../Papilio-Loader/papilio-prog
MISOC_DIRECTORY := $(shell pwd)/../misoc/misoc
MSCDIR := $(shell pwd)/misoc_flipsyfat_papilio_pro
SERIAL := /dev/ttyUSB1

.PHONY: all synth flash term

all: synth flash software

synth:
	flipsyfat/targets/papilio_pro.py

flash:
	papilio-prog -v -f misoc_flipsyfat_papilio_pro/gateware/top.bit \
		-b $(PAPILIO_DIR)/bscan_spi_lx9.bit \
		-a 60000:misoc_flipsyfat_papilio_pro/software/bios/bios.bin

software:
	MSCDIR=$(MSCDIR) CPU=lm32 TRIPLE=lm32-elf \
		BUILDINC_DIRECTORY=$(MSCDIR)/software/include \
		MISOC_DIRECTORY=$(MISOC_DIRECTORY) SERIAL=$(SERIAL) \
		make -C flipsyfat/software/experiment load

term:
	miniterm.py --raw $(SERIAL) 115200
