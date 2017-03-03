#ifndef _BLOCK_H
#define _BLOCK_H

#define BLOCK_SIZE	512

void block_read(uint8_t *buf, uint32_t lba);
void block_write(uint8_t *buf, uint32_t lba);

#endif // _BLOCK_H
