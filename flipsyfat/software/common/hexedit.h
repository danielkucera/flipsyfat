// Little hex editor UI

#ifndef _HEXEDIT_H
#define _HEXEDIT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct hexedit_struct {
    uint8_t* buffer;
    int buffer_size;
    int window_addr;
    int window_width;
    int window_height;
    int cursor_low;
    int cursor_size;
    int esc_state;
    int hex_nybble;
} hexedit_t;

void hexedit_init(hexedit_t* editor, uint8_t* buffer, uint32_t size);
bool hexedit_interact(hexedit_t* editor, uint8_t chr);
void hexedit_print(hexedit_t* editor);

#endif // _HEXEDIT_H
