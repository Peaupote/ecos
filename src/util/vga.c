#include "vga.h"

#include "string.h"

vga_pos terminal_pos = {0, 0};
uint8_t terminal_color;
uint16_t* terminal_buffer;
size_t terminal_tab_size;

void terminal_init(uint16_t* p_buffer)
{
    terminal_tab_size = 4;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = p_buffer;
	terminal_clear();
}

void terminal_clear()
{
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_setcolor(uint8_t color)
{
    terminal_color = color;
}

void terminal_putcentryat(uint16_t e, size_t x, size_t y)
{
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = e;
}
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y)
{
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_nextline()
{
    terminal_pos.x = 0;
    if (++terminal_pos.y == VGA_HEIGHT)
        terminal_pos.y = 0;
}

void terminal_cursor_at(size_t rw, size_t cl)
{
	terminal_pos.x = cl;
	terminal_pos.y = rw;
}

void terminal_putachar(char c)
{
    terminal_putentryat(c, terminal_color,
			terminal_pos.x, terminal_pos.y);
    if (++terminal_pos.x == VGA_WIDTH)
        terminal_nextline();
}

void terminal_putchar(char c)
{
    switch(c){
        case '\n':
            terminal_nextline();
            break;
        case '\t':
            for(size_t i = 0; i < terminal_tab_size; ++i)
                terminal_putachar(' ');
            break;
        default:
            terminal_putachar(c);
    }
}

void terminal_write(const char* data, size_t size)
{
    for (size_t i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

void terminal_writestring(const char* data)
{
    terminal_write(data, ustrlen(data));
}

void terminal_writer(void* none __attribute__((unused)), const char *data)
{
    terminal_write(data, ustrlen(data));
}
