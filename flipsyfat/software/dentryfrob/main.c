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
#include "hexedit.h"

static uint8_t guess[FAT_DENTRY_SIZE];
static int num_files = FAT_MAX_ROOT_ENTRIES - 1;
static bool auto_advance = false;

#define NORMAL_CLKOUT_DIV 16
static const uint32_t reset_gpio_mask = 1 << 0;
static const uint32_t reset_low_len = CONFIG_CLOCK_FREQUENCY / 10;

static void reset_pulse(void)
{
    int ts = 0;
    elapsed(&ts, -1);

    // Hold SD emulator in reset
    sdemu_reset_write(1);

    // Drive reset low, stop clock
    gpio_out_write(gpio_out_read() & ~reset_gpio_mask);
    gpio_oe_write(gpio_oe_read() | reset_gpio_mask);
    clkout_div_write(0);
    while (!elapsed(&ts, reset_low_len));

    // Start clock, emulator, release reset
    clkout_div_write(NORMAL_CLKOUT_DIV);
    sdemu_reset_write(0);
    gpio_oe_write(gpio_oe_read() & ~reset_gpio_mask);
}

static bool local_interact(char ch)
{
    switch (ch) {
        case 'N':
            num_files++;
            return true;
        case 'n':
            num_files--;
            return true;
        case 'A':
            auto_advance = !auto_advance;
            return true;
        case 'R':
            reset_pulse();
            return true;
    }
    return false;
}

int main(void)
{
    hexedit_t editor;

    irq_setmask(0);
    irq_setie(1);
    time_init();
    uart_init();
    sdemu_init();

    reset_pulse();

    puts("Experiment software built "__DATE__" "__TIME__"\n");

    // Init a default directory entry, allow hex-editing any part of it
    fat_plain_file(guess, "DEFAULT", "BIN", 0x100, BLOCK_SIZE * FAT_CLUSTER_SIZE);
    hexedit_init(&editor, guess, sizeof guess);

    while (1) {
        static int last_event = 0;
        static int second_counter_event = 0;
        static int auto_advance_ticks = 0;
        bool force_status = false;

        if (uart_read_nonblock()) {
            uint8_t chr = uart_read();
            force_status |= (editor.esc_state ? 0 : local_interact(chr)) || hexedit_interact(&editor, chr);
        }

        // Add-on for the hex editor, automatic advance timer for time-lapse experiments
        if (elapsed(&second_counter_event, CONFIG_CLOCK_FREQUENCY)) {
            if (auto_advance) {
                if (auto_advance_ticks > 1) {
                    auto_advance_ticks--;
                } else {
                    auto_advance_ticks = 2;
                    hexedit_interact(&editor, '+');
                }
            }
        }

        if (force_status || elapsed(&last_event, CONFIG_CLOCK_FREQUENCY / 4)) {
            printf("\e[H"); // Home
            if (!force_status) printf("\e[J"); // Clear
            hexedit_print(&editor);
            printf("\nauto=%02d nfile=%02x\n",
                auto_advance ? auto_advance_ticks : 0, num_files);
            sdemu_status();
        }
    }

    return 0;
}

void fat_rootdir_entry(uint8_t* dest, unsigned index)
{
    if (index == 0) {
        memset(dest, 0, FAT_DENTRY_SIZE);
        fat_volume_label(dest);
    } else if (index <= num_files) {
        memcpy(dest, guess, sizeof guess);
    } else {
        // Reading end of directory table
        memset(dest, 0, FAT_DENTRY_SIZE);
        sdtrig_latch_write(sdtrig_latch_read() | 0x10);
    }
}

void fat_data_block(uint8_t* dest, unsigned cluster, unsigned index)
{
    memset(dest, 'Z', BLOCK_SIZE); 
    sprintf((char*) dest, "%04x+%x cluster\n", cluster, index);
}
