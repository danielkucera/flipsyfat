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
#include "block_guess.h"


int main(void)
{
    irq_setmask(0);
    irq_setie(1);
    time_init();
    uart_init();
    sdemu_init();

    puts("Blockfrob software built "__DATE__" "__TIME__"\n");

    while (1) {
        static int last_event = 0;
        static int offset = 0;
 
        uint8_t *current = &block_guess[offset];
        bool force_status = false;

        if (uart_read_nonblock()) {

            switch (uart_read()) {
                case 'w':
                    (*current)++;
                    break;
                case 's':
                    (*current)--;
                    break;
                case 'W':
                    memset(current, *current + 1, sizeof block_guess - offset);
                    break;
                case 'S':
                    memset(current, *current - 1, sizeof block_guess - offset);
                    break;
                case 'a':
                    offset--;
                    break;
                case 'd':
                    offset++;
                    break;
                case 'A':
                    offset -= 0x10;
                    break;
                case 'D':
                    offset += 0x10;
                    break;
                default:
                    continue;
            }
            force_status = true;
            if (offset < 0) offset = 0;
            if (offset >= sizeof block_guess) offset = sizeof block_guess - 1;
        }

        if (force_status || elapsed(&last_event, CONFIG_CLOCK_FREQUENCY / 4)) {
            printf("[+%03x]=%02x ", offset, block_guess[offset]);
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
