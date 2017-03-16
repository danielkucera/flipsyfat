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

#define NORMAL_CLKOUT_DIV 16
static const uint32_t normal_measurement_low = 2120 * NORMAL_CLKOUT_DIV;
static const uint32_t normal_measurement_high = 2160 * NORMAL_CLKOUT_DIV;
static const uint32_t max_replicate_count = 32;

static const uint32_t reset_gpio_mask = 1 << 0;

static const uint32_t watchdog_period = CONFIG_CLOCK_FREQUENCY * 4;
static const uint32_t status_period = CONFIG_CLOCK_FREQUENCY / 2;
static const uint32_t reset_low_len = CONFIG_CLOCK_FREQUENCY / 10;
static const uint32_t reset_high_len = CONFIG_CLOCK_FREQUENCY / 10;

static uint32_t reset_counter;
static uint32_t reset_ts;
static bool reset_pending = false;
static bool timer_armed = false;

queue_entry queue[QUEUE_SIZE];
volatile qptr_t qptr_write_guess;
volatile qptr_t qptr_read_guess;
volatile qptr_t qptr_write_measurement;
volatile qptr_t qptr_read_measurement;


void reset_pulse(void)
{
    int ts = 0;

    reset_counter++;
    timer_armed = false;
    reset_pending = false;
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

    // Another delay, then capture a fresh reset timestamp
    while (!elapsed(&ts, reset_high_len));
    sdtimer_capture_write(0);
    reset_ts = sdtimer_capture_ts_read();
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
        printf("Trying [%.8s.%.3s] qptr (%06x-%06x-%06x-%06x) rst=%d ",
            name, name+8,
            (unsigned)(qptr_write_guess & 0xffffff),
            (unsigned)(qptr_read_guess & 0xffffff),
            (unsigned)(qptr_write_measurement & 0xffffff),
            (unsigned)(qptr_read_measurement & 0xffffff),
            reset_counter);
        sdemu_status();
    }
}

static void dequeue_results(void)
{
    while (qptr_read_measurement != qptr_write_measurement) {
        uint32_t measurement = qentry(qptr_read_measurement)->measurement;    

        if (measurement < normal_measurement_low || measurement > normal_measurement_high) {
            // Unusual! Replicate this measurement to be sure

            if ((qptr_write_guess - qptr_read_measurement) > QUEUE_SIZE - 1) {
                // Stop dequeueing until we have room to resubmit this guess
                return;
            }

            printf("Unusual RESULT #%llu rep=%d [%.8s.%.3s] = %d\n",
                (long long unsigned) qptr_read_measurement,
                qentry(qptr_read_measurement)->replicate_count,
                qentry(qptr_read_measurement)->guess,
                qentry(qptr_read_measurement)->guess + 8,
                measurement);

            if (qentry(qptr_read_measurement)->replicate_count + 1 < max_replicate_count) {
                // Replicate this experiment
                memcpy(qentry(qptr_write_guess)->guess, qentry(qptr_read_measurement)->guess, FAT_DENTRY_SIZE);
                qentry(qptr_write_guess)->replicate_count = qentry(qptr_read_measurement)->replicate_count + 1;
                qptr_write_guess++;
            }
        }

        qptr_read_measurement++;
    }
}

void guess_dentry(const uint8_t *dentry)
{
    // Keep the queue half full of normal guesses, leaving room for retries.
    do {
        mainloop_poll();
        dequeue_results();
    } while ((qptr_write_guess - qptr_read_measurement) > QUEUE_SIZE / 2);

    memcpy(qentry(qptr_write_guess)->guess, dentry, FAT_DENTRY_SIZE);
    qentry(qptr_write_guess)->replicate_count = 0;
    qptr_write_guess++;
}

void guess_filename(const char *name, const char *ext)
{
    uint8_t dentry[FAT_DENTRY_SIZE];
    fat_plain_file(dentry, name, ext, 0x100, 0x10000);
    guess_dentry(dentry);
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

#if 0   // Control experiment; all files starting with 'D' should take less time
        if (dest[0] == 'D') {
            dest[0xb] = 0x8;
        }
#endif
    }

    // First entry in sector; measure processing time if the timer was armed
    if (offset == 0 && timer_armed) {
        qentry(qptr_write_measurement)->measurement = sdtimer_read_ts_read() - sdtimer_done_ts_read();
        qptr_write_measurement++;
        timer_armed = false;
    }

    if (index == FAT_MAX_ROOT_ENTRIES - 1) {
        // Last record; reset target to continue the experiment
        reset_pending = true;
        timer_armed = false;

    } else if (offset == FAT_DENTRY_PER_SECTOR - 1 && qptr_read_guess + 1 < qptr_write_guess) {
        // End of sector, more guesses available

        while (qptr_write_measurement < qptr_read_guess) {
            // Skipped masurements; main loop will requeue them, we can't write to that queue safely from the ISR.
            qentry(qptr_write_measurement)->measurement = 0;
            qptr_write_measurement++;
        }

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
