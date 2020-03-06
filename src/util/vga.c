#include "vga.h"

#include "string.h"

vga_pos_t vga_pos = {0, 0};
uint8_t   vga_color;
uint16_t* vga_buffer;
size_t    vga_tab_size;

void vga_init(uint16_t* p_buffer)
{
    vga_tab_size = 4;
    vga_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_buffer = p_buffer;
	vga_clear();
}

void vga_clear()
{
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga_buffer[index] = vga_entry(' ', vga_color);
        }
    }
}

void vga_setcolor(uint8_t color)
{
    vga_color = color;
}

void vga_putcentryat(uint16_t e, size_t x, size_t y)
{
    const size_t index = y * VGA_WIDTH + x;
    vga_buffer[index] = e;
}
void vga_putentryat(char c, uint8_t color, size_t x, size_t y)
{
    const size_t index = y * VGA_WIDTH + x;
    vga_buffer[index] = vga_entry(c, color);
}

void vga_nextline()
{
    vga_pos.x = 0;
    if (++vga_pos.y == VGA_HEIGHT)
        vga_pos.y = 0;
}

void vga_cursor_at(size_t rw, size_t cl)
{
	vga_pos.x = cl;
	vga_pos.y = rw;
}

void vga_putachar(char c)
{
    vga_putentryat(c, vga_color,
			vga_pos.x, vga_pos.y);
    if (++vga_pos.x == VGA_WIDTH)
        vga_nextline();
}

void vga_putchar(char c)
{
    switch(c){
        case '\n':
            vga_nextline();
            break;
        case '\t':
            for(size_t i = 0; i < vga_tab_size; ++i)
                vga_putachar(' ');
            break;
        default:
            vga_putachar(c);
    }
}

void vga_write(const char* data, size_t size)
{
    for (size_t i = 0; i < size; i++)
        vga_putchar(data[i]);
}

void vga_writestring(const char* data)
{
    vga_write(data, ustrlen(data));
}

void vga_writer(void* none __attribute__((unused)), const char *data)
{
    vga_write(data, ustrlen(data));
}
