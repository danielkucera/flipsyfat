#ifndef _SDTIMER_H
#define _SDTIMER_H

#include <stdio.h>
#include <generated/csr.h>

static void sdtimer_status(void)
{
	printf("now=%08x rts=%08x wts=%08x dts=%08x ",
		sdtimer_now_read(), sdtimer_read_ts_read(), sdtimer_write_ts_read(), sdtimer_done_ts_read());
}

#endif
