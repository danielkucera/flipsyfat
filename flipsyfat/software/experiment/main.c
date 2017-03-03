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
#include "block.h"


static void change_guess(int offset, int amount)
{
    char c = filename_guess[offset] + amount;
    memset(filename_guess + offset, c, 11 - offset);
}


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
                default:
                    continue;
            }
            force_status = true;
        }

        if (force_status || elapsed(&last_event, CONFIG_CLOCK_FREQUENCY / 4)) {
            printf("guess:%s offset:%02d ", filename_guess, filename_offset);
            sdemu_status();
        }
    }

    return 0;
}
