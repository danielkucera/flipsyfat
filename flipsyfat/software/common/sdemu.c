#include <stdio.h>
#include <stdint.h>

#include <irq.h>
#include <generated/csr.h>
#include <generated/mem.h>
#include "sdemu.h"

static uint32_t sdemu_read_count = 0;
static uint32_t sdemu_write_count = 0;


void sdemu_init(void)
{
    sdemu_reset_write(1);
    sdemu_ev_enable_write(SDEMU_EV_READ | SDEMU_EV_WRITE);
    irq_setmask(irq_getmask() | (1 << SDEMU_INTERRUPT));
    sdemu_reset_write(0);
}

void sdemu_isr(void)
{
    unsigned int stat;

    stat = sdemu_ev_pending_read();

    if (stat & SDEMU_EV_READ) {
        uint32_t addr = sdemu_read_addr_read();
        block_read((uint8_t *) SDEMU_BASE, addr);
        sdemu_ev_pending_write(SDEMU_EV_READ);
        sdemu_read_count++;
    }

    if (stat & SDEMU_EV_WRITE) {
        uint32_t addr = sdemu_write_addr_read();
        block_write((uint8_t *) (SDEMU_BASE + BLOCK_SIZE), addr);
        sdemu_ev_pending_write(SDEMU_EV_WRITE);
        sdemu_write_count++;
    }
}

void sdemu_status(void)
{
    printf("rd:%08x wr:%08x rda:%08x wra:%08x cardstat:%08x info:%04x cmd:%d\n",
        sdemu_read_count,
        sdemu_write_count,
        sdemu_read_addr_read(),
        sdemu_write_addr_read(),
        sdemu_card_status_read(),
        sdemu_info_bits_read(),
        sdemu_most_recent_cmd_read());
}
