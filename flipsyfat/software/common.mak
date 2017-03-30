SERIAL = /dev/ttyUSB1
BAUDRATE = 500000

PAPILIO_DIR := $(shell pwd)/../../../../Papilio-Loader/papilio-prog
MSCDIR := $(shell pwd)/../../../misoc_flipsyfat_papilio_pro

include $(MSCDIR)/software/include/generated/variables.mak
include $(MISOC_DIRECTORY)/software/common.mak

# Kludge... why isn't gcc finding this on its own?
LIBGCC_DIR=/usr/local/lib/gcc/lm32-elf/6.3.0

COMMON := $(shell pwd)/../common
INCLUDES += -I$(COMMON)
