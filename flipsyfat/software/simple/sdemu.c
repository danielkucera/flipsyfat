#include <stdio.h>
#include <stdint.h>

#include <irq.h>
#include <generated/csr.h>
#include <generated/mem.h>
#include "sdemu.h"
#include "block.h"

static uint32_t sdemu_read_count = 0;
static uint32_t sdemu_write_count = 0;
static uint32_t sdemu_last_addr = 0;


void sdemu_init(void)
{
    sdemu_ev_enable_write(SDEMU_EV_READ | SDEMU_EV_WRITE);
    irq_setmask(irq_getmask() | (1 << SDEMU_INTERRUPT));
}

void sdemu_isr(void)
{
    unsigned int stat;

    stat = sdemu_ev_pending_read();

    if (stat & SDEMU_EV_READ) {
        uint32_t addr = sdemu_read_addr_read();
        block_read((uint8_t *) SDEMU_BASE, addr);
        sdemu_read_go_write(0);
        sdemu_read_count++;
        sdemu_last_addr = addr;
    }

    if (stat & SDEMU_EV_WRITE) {
        uint32_t addr = sdemu_write_addr_read();
        block_write((uint8_t *) (SDEMU_BASE + BLOCK_SIZE), addr);
        sdemu_write_done_write(0);
        sdemu_write_count++;
        sdemu_last_addr = addr;
    }
}

void sdemu_status(void)
{
    printf("rd:%08x wr:%08x addr:%08x\n", sdemu_read_count, sdemu_write_count, sdemu_last_addr);
}
