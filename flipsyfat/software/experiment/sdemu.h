#ifndef _SDEMU_H
#define _SDEMU_H

#define SDEMU_EV_READ   (1 << 0)
#define SDEMU_EV_WRITE  (1 << 1)

void sdemu_isr(void);
void sdemu_init(void);
void sdemu_status(void);

#endif // _SDEMU_H
