#include <stdint.h>
#include <string.h>
#include "colorbars.h"

// Color bar image exported from Gimp as C code, included inline
#include "offline.c"

const uint8_t* get_color_bar_data(void)
{
    return offline_colorbars.pixel_data;
}

void generate_color_bars(uint8_t* buf, int width, int height)
{
    memcpy(buf, get_color_bar_data(), width * height * 4);
}
