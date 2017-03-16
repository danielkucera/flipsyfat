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
#include "sdtimer.h"
#include "guesser.h"

static const uint32_t normal_clkout_div = 16;

static const uint32_t watchdog_period = CONFIG_CLOCK_FREQUENCY * 4;
static const uint32_t status_period = CONFIG_CLOCK_FREQUENCY / 2;
static const uint32_t reset_low_len = CONFIG_CLOCK_FREQUENCY / 10;
static const uint32_t reset_high_len = CONFIG_CLOCK_FREQUENCY / 10;

static const uint32_t reset_gpio_mask = 1 << 0;

static uint32_t reset_counter;
static uint32_t reset_ts;
static bool reset_pending = false;
static bool timer_armed = false;

queue_entry queue[QUEUE_SIZE];
volatile uint32_t qptr_write_guess;
volatile uint32_t qptr_read_guess;
volatile uint32_t qptr_write_measurement;
volatile uint32_t qptr_read_measurement;

void reset_pulse(void)
{
    int ts = 0;

    reset_counter++;
    timer_armed = false;
    reset_pending = false;
    elapsed(&ts, -1);

    // Drive reset low, stop clock
    gpio_out_write(gpio_out_read() & ~reset_gpio_mask);
    gpio_oe_write(gpio_oe_read() | reset_gpio_mask);
    clkout_div_write(0);

    while (!elapsed(&ts, reset_low_len));

    // Start clock, release reset

    clkout_div_write(normal_clkout_div);
    gpio_oe_write(gpio_oe_read() & ~reset_gpio_mask);

    // Another delay, then capture a fresh reset timestamp
    while (!elapsed(&ts, reset_high_len));
    sdtimer_capture_write(0);
    reset_ts = sdtimer_now_read();
}

void mainloop_poll(void)
{
    // Stuff to do while we're waiting

    static int last_event = 0;
    uint32_t now, rdts;

    sdtimer_capture_write(0);
    now = sdtimer_capture_ts_read();
    rdts = sdtimer_read_ts_read();

    if ((int32_t)(now - rdts) > watchdog_period &&
        (int32_t)(now - reset_ts) > watchdog_period) {
        printf("Experiment seems stuck, resetting target.\n");
        reset_pending = true;
    }

    if (reset_pending && (int32_t)(now - reset_ts) > watchdog_period) {
        reset_pulse();
    }

    // Status
    if (elapsed(&last_event, status_period)) {
        uint8_t *name = qentry(qptr_read_guess)->guess;
        printf("Trying [%.8s.%.3s] qptr (%d,%d,%d,%d) rst=%d ",
            name, name+8,
            qptr_write_guess, qptr_read_guess,
            qptr_write_measurement, qptr_read_measurement,
            reset_counter);
        sdemu_status();
    }

    // Dequeue results
    while (qptr_read_measurement != qptr_write_measurement) {
        uint8_t *name = qentry(qptr_read_measurement)->guess;
        uint32_t measurement = qentry(qptr_read_measurement)->measurement;
        printf("RESULT #%d [%.8s.%.3s] = %d\n", qptr_read_measurement, name, name+8, measurement);
        qptr_read_measurement++;
    }
}

void guess_enqueue(void)
{
    while (true) {
        if (((qptr_write_guess + 1 - qptr_read_measurement) % QUEUE_SIZE) == 0) {
            // Wait for more space
            mainloop_poll();
            continue;
        }
        qptr_write_guess++;
        break;        
    }
}

void guess_filename(const char *name, const char *ext)
{
    fat_plain_file(qentry(qptr_write_guess)->guess, name, ext, 0x100, 0x10000);
    guess_enqueue();
}

void fat_rootdir_entry(uint8_t* dest, unsigned index)
{
    unsigned offset = index % FAT_DENTRY_PER_SECTOR;

    if (index == 0) {
        // Volume label is special
        fat_volume_label(dest);
    } else {
        // Replicated copy of this sector's experiment
        memcpy(dest, qentry(qptr_read_guess)->guess, FAT_DENTRY_SIZE);

        if (qptr_read_guess & 1) {
            // xxx control
            dest[0xb] = 0x8;
        }
    }

    // First entry in sector; measure processing time if the timer was armed
    if (offset == 0 && timer_armed) {
        uint32_t read_ts = sdtimer_read_ts_read();
        uint32_t done_ts = sdtimer_done_ts_read();
        uint32_t measurement = read_ts - done_ts;
        qentry(qptr_write_measurement)->measurement = measurement;

gpio_oe_write(2);
gpio_out_write(measurement > 100000 ? 2 : 0);
gpio_out_write(0);

        qptr_write_measurement++;
        timer_armed = false;
    }

    if (index == FAT_MAX_ROOT_ENTRIES - 1) {
        // Last record; reset target to continue the experiment
        reset_pending = true;
        timer_armed = false;

    } else if (offset == FAT_DENTRY_PER_SECTOR - 1 &&
        (int32_t)(qptr_read_guess + 1 - qptr_write_guess) < 0) {

        // Start timing the current guess
        timer_armed = true;

        // Next guess
        qptr_read_guess++;
    }
}

void fat_data_block(uint8_t* dest, unsigned cluster, unsigned index)
{
    memset(dest, 'Z', BLOCK_SIZE); 
    sprintf((char*) dest, "%04x+%x cluster\n", cluster, index);
}
