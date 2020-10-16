#include "../include/visky/colormaps.h"

void vky_load_color_texture()
{
    // Fill VKY_COLOR_TEXTURE from `color_texture.img`.
    char path[1024];
    snprintf(path, sizeof(path), "%s/textures/%s", DATA_DIR, "color_texture.img");
    uint8_t* data = (uint8_t*)read_file(path, NULL);
    memcpy(VKY_COLOR_TEXTURE, data, 256 * 256 * 4);
    FREE(data);
}


void vky_colormap_apply(
    VkyColormap cmap, double vmin, double vmax, uint32_t value_count, const double* values,
    VkyColor* colors)
{
    for (uint32_t i = 0; i < value_count; i++)
    {
        colors[i] = vky_color(cmap, (float)values[i], (float)vmin, (float)vmax, 1);
    }
}
