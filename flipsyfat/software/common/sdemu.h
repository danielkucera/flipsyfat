#ifndef _SDEMU_H
#define _SDEMU_H

#include <stdint.h>

#define BLOCK_SIZE  512

#define SDEMU_EV_READ   (1 << 0)
#define SDEMU_EV_WRITE  (1 << 1)

void sdemu_isr(void);
void sdemu_init(void);
void sdemu_status(void);

// Callbacks
void block_read(uint8_t *buf, uint32_t lba);
void block_write(uint8_t *buf, uint32_t lba);

#endif // _SDEMU_H
