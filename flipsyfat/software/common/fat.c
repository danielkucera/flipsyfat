// Tiny FAT16 emulation

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <generated/csr.h>

#include "fat.h"
#include "sdemu.h"

const char* fat_oem_name = "FAKEDOS";
const char* fat_volume_name = "FLIPSY";
const uint32_t fat_volume_serial = 0xf00d1e55;


void block_read(uint8_t *buf, uint32_t lba)
{
    sdtrig_latch_write(0x01);

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
        unsigned sector = (lba - FAT_TABLE_START) % FAT_SECTORS_PER_TABLE;
        for (int i = 0; i < FAT_ENTRIES_PER_SECTOR; i++) {
            fat_table_entry(buf+i*2, sector*FAT_ENTRIES_PER_SECTOR + i);
        }
        break;
    }

    // Root Directory
    case FAT_ROOT_START ... FAT_ROOT_END: {
        unsigned start = (lba - FAT_ROOT_START) * FAT_DENTRY_PER_SECTOR;
        if (lba == FAT_ROOT_END) {
            sdtrig_latch_write(sdtrig_latch_read() | 0x08);
        } else {
            sdtrig_latch_write(sdtrig_latch_read() | 0x02);
        }
        for (int i = 0; i < FAT_DENTRY_PER_SECTOR; i++) {
            fat_rootdir_entry(buf+i*FAT_DENTRY_SIZE, start+i);
        }
        break;
    }

    // File Clusters
    case FAT_ROOT_END + 1 ... FAT_PARTITION_START + FAT_PARTITION_SIZE - 1: {
        unsigned cluster = 2 + ((lba - FAT_ROOT_END - 1) / FAT_CLUSTER_SIZE);
        unsigned offset = (lba - FAT_ROOT_END - 1) % FAT_CLUSTER_SIZE;
        sdtrig_latch_write(sdtrig_latch_read() | 0x08);
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
