#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "block.h"

#define FAT_PARTITION_START     0x3f
#define FAT_PARTITION_SIZE  	0xf480
#define FAT_CLUSTER_SIZE        4
#define FAT_RESERVED_SECTORS    1
#define FAT_MAX_ROOT_ENTRIES    0x200
#define FAT_SECTORS_PER_TABLE   0x3e
#define FAT_NUM_TABLES          2
#define FAT_DENTRY_PER_SECTOR   16
#define FAT_MEDIA_DESCRIPTOR    0xf8     // Hard Disk
#define FAT_PHYSICAL_DRIVE_NUM  0x80     // DOS drive
#define FAT_SECTORS_PER_TRACK   0x20     // Legacy CHS
#define FAT_CHS_HEADS           0x10
#define FAT_HIDDEN_SECTORS      0x3f
#define FAT_EXT_BOOT_SIGNATURE  0x29     // Dos 4.0-style FAT16 EBPB
#define FAT_FILESYSTEM_TYPE     "FAT16"

#define FAT_TABLE_START         (FAT_PARTITION_START + FAT_RESERVED_SECTORS)
#define FAT_TABLE_END           (FAT_TABLE_START + FAT_SECTORS_PER_TABLE*FAT_NUM_TABLES - 1)
#define FAT_ROOT_START          (FAT_TABLE_END + 1)
#define FAT_ROOT_END            (FAT_ROOT_START + FAT_MAX_ROOT_ENTRIES/16 - 1)

static const char* fat_oem_name = "FAKEDOS";
static const char* fat_volume_name = "FLIPSY";
static const uint32_t fat_volume_serial = 0xf00d1e55;


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

static void fat_uint16(uint8_t* dest, uint16_t value)
{
    dest[0] = value;
    dest[1] = value >> 8;
}

static void fat_uint24(uint8_t* dest, uint32_t value)
{
    dest[0] = value;
    dest[1] = value >> 8;
    dest[2] = value >> 16;
}

static void fat_uint32(uint8_t* dest, uint32_t value)
{
    dest[0] = value;
    dest[1] = value >> 8;
    dest[2] = value >> 16;
    dest[3] = value >> 24;
}

static void fat_boot_signature(uint8_t* block)
{
    block[0x1fe] = 0x55;
    block[0x1ff] = 0xAA;
}

static void fat_partition(uint8_t* entry, uint32_t first_sector, uint32_t size)
{
    entry[0] = 0x00;                    // Status
    fat_uint24(entry+1, 0x000101);      // First CHS
    entry[4] = 0x0b;                    // Type
    fat_uint24(entry+5, 0xfffffe);      // Last CHS
    fat_uint32(entry+8, first_sector);
    fat_uint32(entry+12, size);
}

static void fat_table_entry(uint8_t* dest, unsigned cluster)
{
    if (cluster == 0) {
        // Special case
        fat_uint16(dest, 0xff00 | FAT_MEDIA_DESCRIPTOR);

    } else if (cluster < FAT_PARTITION_SIZE / FAT_CLUSTER_SIZE) {
        // All possible clusters occupied, not chained.
        // No free space.
        fat_uint16(dest, 0xffff);

    } else {
        // Unused table entry
        fat_uint16(dest, 0x0000);
    }
}

static void fat_rootdir_entry(uint8_t* dest, unsigned index)
{
    memset(dest, 0, 32);

    if (index == 0) {
        // Volume label
        fat_string(dest, fat_volume_name, 11);
        dest[0x0b] = 0x28;

    } else {
        char name[12];
        const char* ext = "TXT";

        unsigned filesize = BLOCK_SIZE * FAT_CLUSTER_SIZE;
        unsigned first_cluster = 0x100 + index;

        sprintf(name, "F%d", index);

        fat_string(dest, name, 8);
        fat_string(dest+0x8, ext, 3);
        fat_uint16(dest+0x1a, first_cluster);
        fat_uint32(dest+0x1c, filesize);
    }
}

static void fat_data_block(uint8_t* dest, unsigned cluster, unsigned index)
{
    memset(dest, 'd', BLOCK_SIZE); 
    sprintf((char*) dest, "%04x+%x cluster\n", cluster, index);
}

void block_read(uint8_t *buf, uint32_t lba)
{
	switch (lba) {

    // Master Boot Record
	case 0: {
		memset(buf, 0, BLOCK_SIZE);
        fat_boot_signature(buf);
        fat_partition(buf+0x1be, FAT_PARTITION_START, FAT_PARTITION_SIZE);
		break;
    }

    // Reserved space
    case 1 ... FAT_PARTITION_START - 1: {
        memset(buf, 0, BLOCK_SIZE);
        break;
    }

	// FAT Boot Sector
	case FAT_PARTITION_START: {
		memset(buf, 0, BLOCK_SIZE);
        fat_boot_signature(buf);
        fat_uint24(buf, 0x903ceb);      // Jump to 0x3e
        fat_uint16(buf+0x3e, 0x19cd);   // Int 19h, reboot
        fat_string(buf+0x03, fat_oem_name, 8);
        fat_uint16(buf+0x0b, BLOCK_SIZE);
        buf[0x0d] = FAT_CLUSTER_SIZE;
        fat_uint16(buf+0x0e, FAT_RESERVED_SECTORS);
        buf[0x10] = FAT_NUM_TABLES;
        fat_uint16(buf+0x11, FAT_MAX_ROOT_ENTRIES);
        fat_uint16(buf+0x13, FAT_PARTITION_SIZE);
        buf[0x15] = FAT_MEDIA_DESCRIPTOR;
        fat_uint16(buf+0x16, FAT_SECTORS_PER_TABLE);
        fat_uint16(buf+0x18, FAT_SECTORS_PER_TRACK);
        fat_uint16(buf+0x1a, FAT_CHS_HEADS);
        fat_uint16(buf+0x1c, FAT_HIDDEN_SECTORS);
        buf[0x24] = FAT_PHYSICAL_DRIVE_NUM;
        buf[0x26] = FAT_EXT_BOOT_SIGNATURE;
        fat_uint32(buf+0x27, fat_volume_serial);
        fat_string(buf+0x2b, fat_volume_name, 11);
        fat_string(buf+0x36, FAT_FILESYSTEM_TYPE, 8);
		break;
    }

    // FAT Tables 1 + 2
    case FAT_TABLE_START ... FAT_TABLE_END: {
        const unsigned per_sector = BLOCK_SIZE / 2;
        unsigned sector = (lba - FAT_TABLE_START) % FAT_SECTORS_PER_TABLE;
        for (int i = 0; i < per_sector; i++) {
            fat_table_entry(buf+i*2, sector*per_sector + i);
        }
        break;
    }

    // Root Directory
    case FAT_ROOT_START ... FAT_ROOT_END: {
        unsigned start = (lba - FAT_ROOT_START) * FAT_DENTRY_PER_SECTOR;
        for (int i = 0; i < FAT_DENTRY_PER_SECTOR; i++) {
            fat_rootdir_entry(buf+i*0x20, start+i);
        }
        break;
    }

    // File Clusters
    case FAT_ROOT_END + 1 ... FAT_PARTITION_START + FAT_PARTITION_SIZE - 1: {
        unsigned cluster = 2 + ((lba - FAT_ROOT_END - 1) / FAT_CLUSTER_SIZE);
        unsigned offset = (lba - FAT_ROOT_END - 1) % FAT_CLUSTER_SIZE;
        fat_data_block(buf, cluster, offset);
        break;
    }

	// Default pattern
	default:
		memset(buf, 'x', BLOCK_SIZE);
		sprintf((char*) buf, "%08x block\n", lba);
	}
}

void block_write(uint8_t *buf, uint32_t lba)
{
    // Ignored
}
