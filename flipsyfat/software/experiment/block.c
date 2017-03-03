#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "block.h"

#define FAT_PARTITION_START     0x3f
#define FAT_PARTITION_SIZE  	0xf49e
#define FAT_CLUSTER_SIZE        4
#define FAT_RESERVED_SECTORS    1
#define FAT_MAX_ROOT_ENTRIES    0x200
#define FAT_ROOT_CLUSTER        2
#define FAT_SECTORS_PER_TABLE   0x3e

#define FAT_TABLE_START         (FAT_PARTITION_START + FAT_RESERVED_SECTORS)
#define FAT_TABLE_END           (FAT_TABLE_START + FAT_SECTORS_PER_TABLE*2 - 1)
#define FAT_ROOT_START          (FAT_TABLE_END + 1)
#define FAT_ROOT_END            (FAT_ROOT_START + FAT_MAX_ROOT_ENTRIES/16 - 1)

static const char* fat_volume_name = "FLIPSY";


static void fat_string(uint8_t* dest, const char *cstr, int field_len)
{
    while (field_len && *cstr) {
        *(dest++) = *(cstr++);
        field_len--;
    }
    while (field_len) {
        *(dest++) = ' ';
        field_len--;
    }
}


static void fat_dentry(uint32_t* dest, unsigned index)
{
    uint8_t *bytes = (uint8_t*) dest;
    memset(bytes, 0, 32);

    if (index == 0) {
        // Volume label
        fat_string(bytes, fat_volume_name, 11);
        bytes[0x0b] = 0x28;

    } else {
        char name[12];
        const char* ext = "TXT";

        unsigned filesize = 12345;
        unsigned first_cluster = 0x100 + index;

        sprintf(name, "F%d", index);

        fat_string(bytes, name, 11);
        fat_string(bytes+8, ext, 3);
        bytes[0x1a] = first_cluster & 0xff;
        bytes[0x1b] = first_cluster >> 8;
        dest[7] = filesize;
    }
}


void block_write(uint32_t *buf, uint32_t lba)
{
	// Ignore writes
}


void block_read(uint32_t *buf, uint32_t lba)
{
	uint8_t *bytes = (uint8_t*) buf;

	switch (lba) {

    // Master Boot Record
	case 0: {
		memset(bytes, 0, BLOCK_SIZE);
		buf[111] = 0x00000001;
		buf[112] = 0x01000bfe;
		buf[113] = 0xffff0000 | (__builtin_bswap32(FAT_PARTITION_START) >> 16);
		buf[114] = (__builtin_bswap32(FAT_PARTITION_START) << 16) | (__builtin_bswap32(FAT_PARTITION_SIZE) >> 16);
		buf[127] = 0x000055aa | (__builtin_bswap32(FAT_PARTITION_SIZE) << 16);
		break;
    }

    // Reserved space
    case 1 ... FAT_PARTITION_START - 1: {
        memset(bytes, 0, BLOCK_SIZE);
        break;
    }

	// FAT Boot Sector
	case FAT_PARTITION_START: {
		memset(bytes, 0, BLOCK_SIZE);
        buf[3] = 0x02000000 | (FAT_CLUSTER_SIZE << 16) | (__builtin_bswap32(FAT_RESERVED_SECTORS) >> 16);
        buf[4] = 0x02000000 | ((__builtin_bswap32(FAT_MAX_ROOT_ENTRIES) >> 16) << 8);
        buf[5] = 0x00F80000 | (__builtin_bswap32(FAT_SECTORS_PER_TABLE) >> 16);
        buf[8] = __builtin_bswap32(FAT_PARTITION_SIZE);
        buf[9] = 0x20001000;
        buf[10] = 0x3f000000;
        buf[11] = __builtin_bswap32(FAT_ROOT_CLUSTER);
        fat_string(bytes+43, fat_volume_name, 11);
        fat_string(bytes+54, "FAT16", 8);
		buf[127] = 0x000055aa;
		break;
    }

    // FAT Tables 1 + 2
    case FAT_TABLE_START ... FAT_TABLE_END: {
        unsigned sector = (lba - FAT_TABLE_START) % FAT_SECTORS_PER_TABLE;
        memset(bytes, 0, BLOCK_SIZE);
        if (sector == 0) {
            buf[0] = 0xf8ffffff;
            buf[1] = 0xffffffff;
            buf[2] = 0xffffffff;
        }
        break;
    }

    // Root Directory
    case FAT_ROOT_START ... FAT_ROOT_END: {
        for (int i = 0; i < 16; i++) {
            fat_dentry(buf+i*8, (lba - FAT_ROOT_START) * 16 + i);
        }
        break;
    }

	// Default pattern
	default:
		memset(bytes, 'x', BLOCK_SIZE);
		sprintf((char*)bytes, "%08x\n", lba);
	}
}

