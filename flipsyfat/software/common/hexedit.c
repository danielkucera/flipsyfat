#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <uart.h>
#include "hexedit.h"

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif


static void hexedit_validate(hexedit_t* editor)
{
    uint32_t win_bytes, cursor_max;

    editor->window_width = MAX(1, editor->window_width);
    editor->window_height = MAX(1, MIN((editor->buffer_size + editor->window_width - 1) / editor->window_width, editor->window_height));
    win_bytes = editor->window_width * editor->window_height;

    editor->window_addr = MAX(0, MIN(editor->buffer_size - win_bytes, editor->window_addr));
    editor->cursor_low = MAX(0, MIN(editor->buffer_size - 1, editor->cursor_low));
    editor->cursor_data_width = MAX(1, editor->cursor_data_width);

    cursor_max = editor->buffer_size - editor->cursor_low;
    editor->cursor_data_width = MAX(1, MIN(cursor_max, editor->cursor_data_width));
    editor->cursor_size = MAX(editor->cursor_data_width, MIN(cursor_max, editor->cursor_size));

    while (editor->cursor_low < editor->window_addr && editor->window_addr >= editor->window_width) {
        editor->window_addr -= editor->window_width;
    }

    while (editor->cursor_low + editor->cursor_size > editor->window_addr + win_bytes &&
        editor->window_addr + editor->window_width < editor->buffer_size) {
        editor->window_addr += editor->window_width;
    }
}

void hexedit_init(hexedit_t* editor, uint8_t* buffer, uint32_t size)
{
    memset(editor, 0, sizeof *editor);
    editor->buffer = buffer;
    editor->buffer_size = size;
    editor->window_width = 16;
    editor->window_height = 8;
    editor->hex_nybble = -1;
    hexedit_validate(editor);
}

static int enter_nybble(hexedit_t* editor, uint8_t n)
{
    if (editor->hex_nybble < 0) {
        // First nybble
        editor->hex_nybble = n;
        return -1;
    } else {
        // Complete byte
        int prev = editor->hex_nybble;
        editor->hex_nybble = -1;
        return (prev << 4) | n;
    }
}

bool hexedit_interact(hexedit_t* editor, uint8_t chr)
{
    int delta = 0;
    int entered_byte = -1;

    switch (editor->esc_state) {

        case 0:
            switch (chr) {

                case '\e':
                    editor->hex_nybble = -1;
                    editor->esc_state = 1;
                    return false;

                case '0' ... '9':
                    entered_byte = enter_nybble(editor, chr - '0');
                    break;

                case 'a' ... 'f':
                    entered_byte = enter_nybble(editor, 10 + chr - 'a');
                    break;

                case '+':
                case 'w':
                    delta = 1;
                    break;

                case 'W':
                    delta = 16;
                    break;

                case '-':
                case 's':
                    delta = -1;
                    break;

                case 'S':
                    delta = -16;
                    break;

                case 'z':
                    editor->window_height--;
                    break;

                case 'Z':
                    editor->window_height++;
                    break;

                case '[':
                    editor->cursor_size--;
                    break;

                case ']':
                    editor->cursor_size++;
                    break;

                case '{':
                    editor->cursor_data_width--;
                    break;

                case '}':
                    editor->cursor_data_width++;
                    break;

                default:
                    return false;
            }
            break;

        case 1:
            editor->esc_state = (chr == '[') ? 2 : 0;
            return false;

        case 2:
            editor->esc_state = 0;
            switch (chr) {

                case 'A':   // Up
                    editor->cursor_low -= editor->window_width;
                    break;

                case 'B':   // Down
                    editor->cursor_low += editor->window_width;
                    break;

                case 'C':   // Right
                    editor->cursor_low++;
                    break;

                case 'D':   // Left
                    editor->cursor_low--;
                    break;

                default:
                    return false;    
            }    
            break;
    }

    hexedit_validate(editor);

    if (delta != 0) {

        // Arbitrary width little endian add
        int carry = delta;
        for (int i = 0; i < editor->cursor_data_width; i++) {
            uint8_t *p = &editor->buffer[editor->cursor_low + i];
            int v = *p + carry;
            carry = v >> 8;
            *p = (uint8_t) v;
        }        

        // Replicate across whole cursor width
        for (int dest = editor->cursor_low + editor->cursor_data_width;
             dest < editor->cursor_size;
             dest += editor->cursor_data_width) {
            memmove(editor->buffer + dest, editor->buffer + editor->cursor_low,
                MIN(editor->cursor_data_width, editor->cursor_size + editor->cursor_low - dest));
        }
    }

    if (entered_byte >= 0) {
        memset(editor->buffer + editor->cursor_low, entered_byte, editor->cursor_size);
        editor->cursor_size--;
        editor->cursor_low++;
        hexedit_validate(editor);
    }

    return true;
}

static char interbyte_char(hexedit_t* editor, uint32_t pre_addr, uint32_t post_addr)
{
    if (pre_addr == editor->cursor_low)
        return '[';

    if (post_addr == editor->cursor_low + editor->cursor_size - 1)
        return ']';

    if ((pre_addr - editor->cursor_low) < editor->cursor_data_width &&
        (post_addr - editor->cursor_low) < editor->cursor_data_width)
        return '-';

    return ' ';
}

void hexedit_print(hexedit_t* editor)
{
    for (uint16_t y = 0; y < editor->window_height; y++) {
        uint32_t addr = editor->window_addr + editor->window_width * y;
        printf("%8x: %c", addr, interbyte_char(editor, addr, -1));

        for (uint16_t x = 0; x < editor->window_width; x++) {
            uint32_t xaddr = addr + x;
            uint32_t caddr = xaddr - editor->cursor_low;
            if (editor->hex_nybble >= 0 && caddr < editor->cursor_size) {
                printf("%x_", editor->hex_nybble);
            } else {
                uint8_t chr = editor->buffer[xaddr];
                printf("%02x", chr);
            }
            putchar(interbyte_char(editor, xaddr + 1, xaddr));
        }

        for (uint16_t x = 0; x < editor->window_width; x++) {
            uint8_t chr = editor->buffer[addr + x];
            putchar((chr < 0x80 && isprint(chr)) ? chr : '.');
        }

        putchar('\n');
    }
}
