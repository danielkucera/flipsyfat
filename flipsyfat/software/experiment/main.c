#include <stdio.h>
#include <stdlib.h>

#include <irq.h>
#include <uart.h>
#include <time.h>
#include <generated/csr.h>
#include <generated/mem.h>
#include <hw/flags.h>
#include <console.h>
#include <system.h>

int main(void)
{
	irq_setmask(0);
	irq_setie(1);
	uart_init();

	puts("Experiment software built "__DATE__" "__TIME__"\n");

	while (1) {
	}

	return 0;
}
