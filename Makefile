PAPILIO_DIR := $(shell pwd)/../Papilio-Loader/papilio-prog
SOFTWARES := $(wildcard flipsyfat/software/*)
DEFAULT_SOFTWARE := flipsyfat/software/simple

.PHONY: all synth flash clean $(SOFTWARES)

all: synth flash $(DEFAULT_SOFTWARE)

synth:
	flipsyfat/targets/papilio_pro.py

flash:
	sudo papilio-prog -v -r \
		-f misoc_flipsyfat_papilio_pro/gateware/top.bit \
		-b $(PAPILIO_DIR)/bscan_spi_lx9.bit \
		-a 60000:misoc_flipsyfat_papilio_pro/software/bios/bios.bin

$(SOFTWARES):
	make -C $@ load

clean:
	rm -Rf misoc_flipsyfat_*
