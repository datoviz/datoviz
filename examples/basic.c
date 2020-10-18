#include "../include/visky/visky.h"

#define TRIANGLE 0
#define SCATTER  1
#define PLOT     2
#define IMSHOW   3

#define EXAMPLE IMSHOW

int main()
{

#if EXAMPLE == TRIANGLE
    // Triangle example
    vec3 positions[] = {{-1, -1, 0}, {+1, -1, 0}, {0, +1, 0}};
    VkyColor colors[] = {{{255, 0, 0}, 255}, {{0, 255, 0}, 255}, {{0, 0, 255}, 255}};
    vky_basic_mesh(3, positions, colors);
    vky_basic_run();

#elif EXAMPLE == SCATTER
    // Scatter example
    const uint32_t n = 10000;

    vec3* positions = calloc(n, sizeof(vec3));
    VkyColor* colors = calloc(n, sizeof(VkyColor));
    float* sizes = calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
    {
        positions[i][0] = .25f * randn();
        positions[i][1] = .25f * randn();
        colors[i] = vky_color(VKY_CMAP_VIRIDIS, rand_float(), 0, 1, rand_float());
        sizes[i] = 5 + 25 * rand_float();
    }

    vky_basic_scatter(n, positions, colors, sizes);
    vky_basic_run();
    free(sizes);
    free(colors);
    free(positions);

#elif EXAMPLE == PLOT
    // Plot example
    const uint32_t n = 10000;

    vec3* positions = calloc(n, sizeof(vec3));
    VkyColor* colors = calloc(n, sizeof(VkyColor));
    for (uint32_t i = 0; i < n; i++)
    {
        positions[i][0] = -1 + 2 * (float)i / (n - 1);
        positions[i][1] = .1 * (2 + sin(20 * (float)i / n)) * cos(200 * (float)i / n);
        colors[i] = vky_color(VKY_CMAP_VIRIDIS, i, 0, n, 1);
    }

    vky_basic_plot(n, positions, colors);
    vky_basic_run();
    free(colors);
    free(positions);

#elif EXAMPLE == IMSHOW
    // Imshow example.
    const uint32_t size = 512;

    uint8_t* pixels = calloc(size * size * 4, sizeof(uint8_t));
    float x = 0, y = 0, z = 0;
    VkyColor color = {0};
    for (uint32_t i = 0; i < size; i++)
    {
        x = -1 + 2 * (float)i / (size - 1);
        for (uint32_t j = 0; j < size; j++)
        {
            y = -1 + 2 * (float)j / (size - 1);
            z = exp(-x * x - y * y) * cos(15 * x) * sin(10 * y);
            color = vky_color(VKY_CMAP_VIRIDIS, z, -1, 1, 1);
            pixels[4 * size * i + 4 * j + 0] = color.rgb[0];
            pixels[4 * size * i + 4 * j + 1] = color.rgb[1];
            pixels[4 * size * i + 4 * j + 2] = color.rgb[2];
            pixels[4 * size * i + 4 * j + 3] = color.alpha;
        }
    }

    vky_basic_imshow(size, size, pixels);
    vky_basic_run();
    free(pixels);

#endif

    return 0;
}
