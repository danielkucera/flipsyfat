#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <irq.h>
#include <uart.h>
#include <time.h>
#include <generated/csr.h>
#include <console.h>
#include <system.h>

#include "fat.h"
#include "sdemu.h"


int main(void)
{
    irq_setmask(0);
    irq_setie(1);
    time_init();
    uart_init();
    sdemu_init();

    puts("Simple example software built "__DATE__" "__TIME__"\n");

    while (1) {
        static int last_event = 0;
        if (elapsed(&last_event, CONFIG_CLOCK_FREQUENCY / 10)) {
            sdemu_status();
        }
    }

    return 0;
}

void fat_rootdir_entry(uint8_t* dest, unsigned index)
{
    memset(dest, 0, 32);
    if (index == 0) {
        fat_volume_label(dest);
    } else {
        char name[9];

        // Make up a filename!
        sprintf(name, "F%d", index);

        // Each file will be one cluster, at an arbitrary address
        fat_plain_file(dest, name, "TXT", 0x100 + index, BLOCK_SIZE * FAT_CLUSTER_SIZE);
    }
}

void fat_data_block(uint8_t* dest, unsigned cluster, unsigned index)
{
    memset(dest, 'Z', BLOCK_SIZE); 
    sprintf((char*) dest, "%04x+%x cluster\n", cluster, index);
}
