#include <stdio.h>
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
#include "hexedit.h"
#include "block_guess.h"

int main(void)
{
    hexedit_t editor;

    irq_setmask(0);
    irq_setie(1);
    time_init();
    uart_init();
    sdemu_init();
    hexedit_init(&editor, block_guess, sizeof block_guess);

    puts("Blockfrob software built "__DATE__" "__TIME__"\n");

    while (1) {
        static int last_event = 0;
        bool force_status = false;

        if (uart_read_nonblock()) {
            force_status |= hexedit_interact(&editor, uart_read());
        }

        if (force_status || elapsed(&last_event, CONFIG_CLOCK_FREQUENCY / 2)) {
            printf("\e[H"); // Home
            if (!force_status) printf("\e[J"); // Clear
            hexedit_print(&editor);
            putchar('\n');
            sdemu_status();
        }
    }

    return 0;
}


void block_read(uint8_t *buf, uint32_t lba)
{
    memcpy(buf, block_guess, sizeof block_guess);
    sdtrig_latch_write(0x01 | 
        (lba == 0x0000 ? 0x02 : 0x00) |
        (lba == 0x7e00 ? 0x04 : 0x00) );
}


void block_write(uint8_t *buf, uint32_t lba)
{
    // Ignored
}
