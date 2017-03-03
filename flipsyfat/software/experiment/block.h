#ifndef _BLOCK_H
#define _BLOCK_H

#define BLOCK_SIZE	512

void block_read(uint32_t *buf, uint32_t lba);
void block_write(uint32_t *buf, uint32_t lba);

#endif // _BLOCK_H
