#include <stdio.h>
#include <stdint.h>

#include <irq.h>
#include <generated/csr.h>
#include <generated/mem.h>
#include <hw/flags.h>
#include "sdemu.h"


void sdemu_init(void)
{
    sdemu_read_go_write(0);
    sdemu_write_done_write(0);

    sdemu_ev_pending_write(uart_ev_pending_read());
    sdemu_ev_enable_write(SDEMU_EV_READ | SDEMU_EV_WRITE);
    irq_setmask(irq_getmask() | (1 << SDEMU_INTERRUPT));
}

static void sdemu_finish_read(void)
{
    sdemu_ev_pending_write(SDEMU_EV_READ);
    sdemu_read_go_write(1);
    sdemu_read_go_write(0);
}

static void sdemu_finish_write(void)
{
    sdemu_ev_pending_write(SDEMU_EV_WRITE);
    sdemu_write_done_write(1);
    sdemu_write_done_write(0);
}

void sdemu_isr(void)
{
    unsigned int stat;

    stat = sdemu_ev_pending_read();

    if (stat & SDEMU_EV_READ) {
        putchar('r');
        (*(volatile uint32_t *) SDEMU_BASE)++;
        sdemu_finish_read();
    }

    if (stat & SDEMU_EV_WRITE) {
        putchar('W');
        sdemu_finish_write();
    }
}
