// Tiny FAT16 emulation

#ifndef _FAT_H
#define _FAT_H

#include <stdint.h>
#include <string.h>

#define FAT_PARTITION_START     0x3f
#define FAT_PARTITION_SIZE      0xf480
#define FAT_CLUSTER_SIZE        4
#define FAT_RESERVED_SECTORS    1
#define FAT_MAX_ROOT_ENTRIES    0x1000
#define FAT_SECTORS_PER_TABLE   0x3e
#define FAT_NUM_TABLES          2
#define FAT_DENTRY_SIZE         0x20
#define FAT_DENTRY_PER_SECTOR   0x10
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

extern const char* fat_oem_name;
extern const char* fat_volume_name;
extern const uint32_t fat_volume_serial;

// Callbacks
extern void fat_rootdir_entry(uint8_t* dest, unsigned index);
extern void fat_data_block(uint8_t* dest, unsigned cluster, unsigned index);


static inline void fat_string(uint8_t* dest, const char *cstr, int field_len)
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

static inline void fat_uint16(uint8_t* dest, uint16_t value)
{
    dest[0] = value;
    dest[1] = value >> 8;
}

static inline void fat_uint24(uint8_t* dest, uint32_t value)
{
    dest[0] = value;
    dest[1] = value >> 8;
    dest[2] = value >> 16;
}

static inline void fat_uint32(uint8_t* dest, uint32_t value)
{
    dest[0] = value;
    dest[1] = value >> 8;
    dest[2] = value >> 16;
    dest[3] = value >> 24;
}

static inline void fat_boot_signature(uint8_t* block)
{
    block[0x1fe] = 0x55;
    block[0x1ff] = 0xAA;
}

static inline void fat_partition(uint8_t* entry, uint32_t first_sector, uint32_t size)
{
    entry[0] = 0x00;                    // Status
    fat_uint24(entry+1, 0x000101);      // First CHS
    entry[4] = 0x0b;                    // Type
    fat_uint24(entry+5, 0xfffffe);      // Last CHS
    fat_uint32(entry+8, first_sector);
    fat_uint32(entry+12, size);
}

static inline void fat_table_entry(uint8_t* dest, unsigned cluster)
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

static inline void fat_volume_label(uint8_t *dest)
{
    memset(dest, 0, FAT_DENTRY_SIZE);
    fat_string(dest, fat_volume_name, 11);
    dest[0x0b] = 0x28;
}

static inline void fat_plain_file(uint8_t *dest, const char *name, const char *ext,
    unsigned first_cluster, unsigned filesize)
{
    memset(dest, 0, FAT_DENTRY_SIZE);
    fat_string(dest, name, 8);
    fat_string(dest+0x08, ext, 3);
    fat_uint16(dest+0x1a, first_cluster);
    fat_uint32(dest+0x1c, filesize);
}

#endif // _FAT_H
