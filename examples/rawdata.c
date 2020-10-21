#include "../include/visky/visky.h"

#define BYTES_PER_SAMPLE 2
#define DTYPE            int16_t


typedef struct RawData RawData;
struct RawData
{
    FILE* fp;
    double sample_rate;
    int64_t n_channels;
    int64_t n_samples_total;
    int64_t n_samples_buffer;
    int64_t current_sample;
    DTYPE* buffer;
};

static RawData data = {
    .sample_rate = 30000.0,
    .n_channels = 385,
    .n_samples_buffer = 400,
};


static void load_raw_data(double time)
{
    int64_t bufsize = data.n_samples_buffer;
    ASSERT(bufsize % BYTES_PER_SAMPLE == 0);
    int64_t sample = (int64_t)(round(data.sample_rate * time));
    if (sample < bufsize / 2)
        sample = bufsize / 2;
    else if (sample >= data.n_samples_total - bufsize / 2)
        sample = data.n_samples_total - bufsize / 2;

    int64_t s0 = sample - bufsize / 2;
    int64_t s1 = sample + bufsize / 2;
    int64_t length = s1 - s0;

    ASSERT(s0 <= data.n_samples_total - bufsize);
    ASSERT(s1 < data.n_samples_total - bufsize / 2);
    ASSERT(s0 + s1 == bufsize);

    long offset = (int)(s0 * data.n_channels * BYTES_PER_SAMPLE);
    size_t n_bytes = (size_t)(BYTES_PER_SAMPLE * data.n_channels * length);
    log_debug("load %d bytes from byte %d", n_bytes, offset);

    fseek(data.fp, offset, SEEK_SET);
    size_t n_bytes_read = fread(data.buffer, 1, n_bytes, data.fp);
    ASSERT(n_bytes_read == n_bytes);
}

static void open_raw_data(char* path)
{
    data.fp = fopen(path, "rb");
    if (!data.fp)
    {
        log_error("could not find %s.", path);
        return;
    }

    ASSERT(data.n_channels > 0);
    ASSERT(data.n_samples_buffer > 0);

    // Find the length.
    fseek(data.fp, 0, SEEK_END);
    int64_t length = (int64_t)ftell(data.fp);
    data.n_samples_total = length / (BYTES_PER_SAMPLE * data.n_channels);
    ASSERT(data.n_samples_total > 0);

    data.buffer = calloc((size_t)(data.n_channels * data.n_samples_buffer), sizeof(DTYPE));

    load_raw_data(0);
}

static void close_raw_data()
{
    FREE(data.buffer);
    fclose(data.fp);
}


int main(int argc, char** argv)
{
    log_set_level_env();

    if (argc <= 1)
    {
        log_error("please specify a raw data binary file as command-line argument");
        return 1;
    }

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_BLACK, 1, 1);
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_PANZOOM, NULL);

    VkyTextureParams tex_params =
        vky_default_texture_params(data.n_channels, data.n_samples_buffer, 1);
    tex_params.format_bytes = 2;
    tex_params.format = VK_FORMAT_R16_SNORM;

    VkyImageCmapParams params = {6, 500, 1, &tex_params};
    VkyVisual* visual = vky_visual(scene, VKY_VISUAL_IMAGE_CMAP, &params, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    open_raw_data(argv[1]);

    vky_visual_image_upload(visual, data.buffer);

    vky_visual_data_set_size(visual, 1, 0, NULL, NULL);
    vky_visual_data(visual, VKY_VISUAL_PROP_POS_GPU, 0, 1, (vec3[]){{-1, -1, 0}});
    vky_visual_data(visual, VKY_VISUAL_PROP_POS_GPU, 1, 1, (vec3[]){{+1, +1, 0}});
    vky_visual_data(visual, VKY_VISUAL_PROP_TEXTURE_COORDS, 0, 1, (vec2[]){{0, 0}});
    vky_visual_data(visual, VKY_VISUAL_PROP_TEXTURE_COORDS, 1, 1, (vec2[]){{1, 1}});

    vky_run_app(app);
    vky_destroy_app(app);

    return 0;
}
