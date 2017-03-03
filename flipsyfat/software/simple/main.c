#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <irq.h>
#include <uart.h>
#include <time.h>
#include <generated/csr.h>
#include <generated/mem.h>
#include <hw/flags.h>
#include <console.h>
#include <system.h>
#include "sdemu.h"

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
        if (elapsed(&last_event, CONFIG_CLOCK_FREQUENCY / 10)) {
            sdemu_status();
        }
    }

    return 0;
}
