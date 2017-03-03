#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "block.h"

void block_write(uint32_t *buf, uint32_t lba)
{
	// Ignore writes
}

void block_read(uint32_t *buf, uint32_t lba)
{
	uint8_t *bytes = (uint8_t*) buf;
	switch (lba) {

	case 0:
		memset(bytes, 0, BLOCK_SIZE);
		bytes[510] = 0x55;	// Signature
		bytes[511] = 0xAA;
		
		break;

	default:
		memset(bytes, 'x', BLOCK_SIZE);
		sprintf((char*)bytes, "%08x\n", lba);
	}
}

