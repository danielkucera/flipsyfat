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
#include "fat.h"
#include "hexedit.h"

#define FILE_CLUSTER  0x100
static const char *file_name = "UP_BM";
static const char *file_ext = "BIN";
static uint8_t file_data[0x1000];  // 0x819 seems to be minimum
static volatile uint32_t file_last_read = -1;

#define NORMAL_CLKOUT_DIV 16
static const uint32_t reset_gpio_mask = 1 << 0;
static const uint32_t reset_low_len = CONFIG_CLOCK_FREQUENCY / 10;

static void reset_pulse(void)
{
    int ts = 0;
    elapsed(&ts, -1);

    fat_trace_buffer_index = 0;
    file_last_read = -1;

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
    hexedit_init(&editor, file_data, sizeof file_data);

    puts("File editor software built "__DATE__" "__TIME__"\n");

    reset_pulse();

    while (1) {
        static int last_event = 0;
        bool force_status = false;

        if (uart_read_nonblock()) {
            uint8_t chr = uart_read();
            force_status |= (editor.esc_state ? 0 : local_interact(chr)) || hexedit_interact(&editor, chr);
        }

        if (force_status || elapsed(&last_event, CONFIG_CLOCK_FREQUENCY / 2)) {
            // Home
            printf("\e[H");

            // Clear
            if (!force_status) printf("\e[J");

            printf("last_read=%08x \n", file_last_read);

            printf("trace [");
            for (int i = 0; i < fat_trace_buffer_index; i++) {
                printf(" %x", fat_trace_buffer_block[i]);
            }
            printf(" ]\n\n");

            hexedit_print(&editor);
            putchar('\n');
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
    } else if (index == 1) {
        fat_plain_file(dest, file_name, file_ext, FILE_CLUSTER, sizeof file_data);
    }
}

void fat_data_block(uint8_t* dest, unsigned cluster, unsigned index)
{
    index += (cluster - FILE_CLUSTER) * FAT_CLUSTER_SIZE;
    file_last_read = index * BLOCK_SIZE;
        
    if (index < sizeof file_data / BLOCK_SIZE) {
        memcpy(dest, file_data + index * BLOCK_SIZE, BLOCK_SIZE);
        sdtrig_latch_write(sdtrig_latch_read() | 0x10);
    }
    else {
        memset(dest, 'Z', BLOCK_SIZE); 
    }
}
