#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <irq.h>
#include <uart.h>
#include <time.h>
#include <generated/csr.h>
#include <console.h>
#include <system.h>

#include "sdemu.h"
#include "sdtimer.h"


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
            sdtimer_status();
            sdemu_status();
        }
    }

    return 0;
}

void block_read(uint8_t *buf, uint32_t lba)
{
    // Ignored
}

void block_write(uint8_t *buf, uint32_t lba)
{
    // Ignored
}
