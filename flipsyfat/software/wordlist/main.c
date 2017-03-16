#include <stdio.h>
#include <stdint.h>

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

int main(void)
{
    irq_setmask(0);
    irq_setie(1);
    time_init();
    uart_init();
    sdemu_init();

    puts("Wordlist experiment built "__DATE__" "__TIME__"\n");

    reset_pulse();

    for (char p3 = ' '; p3 <= '`'; p3++) {
    for (char p2 = ' '; p2 <= '`'; p2++) {
    for (char p1 = ' '; p1 <= '`'; p1++) {
    for (char p0 = ' '; p0 <= '`'; p0++) {
        char guess[11] = {p0, p1, p2, p3, p0, p1, p2, p3, p0, p1, p2};
        guess_filename(guess, guess+8);
    }}}}

    return 0;
}
