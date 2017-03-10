#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <irq.h>
#include <uart.h>
#include <time.h>
#include <generated/csr.h>
#include <generated/mem.h>
#include <console.h>
#include <system.h>

#include "sdemu.h"
#include "fat.h"

static char filename_guess[12] = "ABCDEFGHDAT";
void change_guess(int offset, int amount);

static int num_files = FAT_MAX_ROOT_ENTRIES - 1;


int main(void)
{
    irq_setmask(0);
    irq_setie(1);
    time_init();
    uart_init();
    sdemu_init();

    puts("Experiment software built "__DATE__" "__TIME__"\n");

    while (1) {
        static int last_event = 0;
        static int filename_offset = 0;
        bool force_status = false;

        if (uart_read_nonblock()) {
            switch (uart_read()) {
                case 'w':
                    change_guess(filename_offset, 1);
                    break;
                case 's':
                    change_guess(filename_offset, -1);
                    break;
                case 'a':
                    if (filename_offset > 0)
                        filename_offset--;
                    break;
                case 'd':
                    if (filename_offset < 10)
                        filename_offset++;
                    break;
                case 'N':
                    num_files++;
                    break;
                case 'n':
                    num_files--;
                    break;
                default:
                    continue;
            }
            force_status = true;
        }

        if (force_status || elapsed(&last_event, CONFIG_CLOCK_FREQUENCY / 4)) {
            printf("guess:\"%s\" [+%2d]=%02x nfile=%02x ",
                filename_guess, filename_offset, filename_guess[filename_offset], num_files);
            sdemu_status();
        }
    }

    return 0;
}


void change_guess(int offset, int amount)
{
    char c = filename_guess[offset] + amount;
    memset(filename_guess + offset, c, 11 - offset);
}


void fat_rootdir_entry(uint8_t* dest, unsigned index)
{
    memset(dest, 0, 32);
    if (index == 0) {
        fat_volume_label(dest);
    } else if (index <= num_files) {
        fat_plain_file(dest, filename_guess, filename_guess + 8,
            0x100 + index, BLOCK_SIZE * FAT_CLUSTER_SIZE);
    } else {
        // Reading end of directory table
        sdtrig_latch_write(sdtrig_latch_read() | 0x10);
    }
}


void fat_data_block(uint8_t* dest, unsigned cluster, unsigned index)
{
    memset(dest, 'Z', BLOCK_SIZE); 
    sprintf((char*) dest, "%04x+%x cluster\n", cluster, index);
}
