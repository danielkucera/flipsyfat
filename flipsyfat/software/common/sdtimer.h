#ifndef _SDTIMER_H
#define _SDTIMER_H

#include <stdio.h>
#include <generated/csr.h>

static inline void sdtimer_status(void)
{
	sdtimer_capture_write(0);
	printf("now=%08x rts=%08x wts=%08x dts=%08x ",
		sdtimer_capture_ts_read(), sdtimer_read_ts_read(), sdtimer_write_ts_read(), sdtimer_done_ts_read());
}

#endif
