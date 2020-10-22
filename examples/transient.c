#include <math.h>

#include "../include/visky/visky.h"

static VkyMarkersTransientParams params = {0};
static VkyVisual* visual = NULL;
static float* last_active = NULL;
static uint32_t n_clusters = 0;
static float last_time = 0;
static uint64_t i0 = 0;
static double* spike_times = NULL;
static double* spike_amps = NULL;
static float* point_size = NULL;
static int32_t* spike_clusters;
static uint32_t n_spikes = 0;

static void frame_callback(VkyCanvas* canvas, void* data)
{
    params.local_time = canvas->local_time;
    vky_visual_params(visual, sizeof(params), &params);

    // Find the clusters that spiked since the last frame.
    float t0 = last_time;
    float t1 = params.local_time;
    float t = 0;
    int32_t cl = 0;
    for (uint32_t i = i0; i < n_spikes; i++)
    {
        t = spike_times[i];
        if (t < t0)
            i0 = i;
        else if (t >= t1)
            break;
        if (t0 <= t && t < t1)
        {
            cl = spike_clusters[i];
            last_active[cl] = t1;
            point_size[cl] = 200000 * spike_amps[i];
        }
    }

    for (uint32_t i = 0; i < n_clusters; i++)
    {
        vky_visual_data(visual, VKY_VISUAL_PROP_TIME, 0, n_clusters, last_active);
        vky_visual_data(visual, VKY_VISUAL_PROP_SIZE, 0, n_clusters, point_size);
    }
    last_time = params.local_time;
}


int main()
{
    log_set_level_env();

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_BLACK, 1, 1);
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    vky_set_controller(panel, VKY_CONTROLLER_ARCBALL, NULL);

    // Create the visual.
    visual = vky_visual(scene, VKY_VISUAL_MARKER_TRANSIENT, &params, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);



    // Load the files.
    size_t size = 0;

    // Channel colors.
    cvec3* channel_colors =
        (cvec3*)read_npy("/home/cyrille/git/visky/data/misc/channels.colors.npy", &size);
    ASSERT(size % 3 == 0); // RGB
    uint32_t n_channels = size / 3;

    // Channel positions.
    vec3* channel_positions =
        (vec3*)read_npy("/home/cyrille/git/visky/data/misc/channels.positions.npy", &size);

    // Cluster channels.
    int64_t* cluster_channels =
        (int64_t*)read_npy("/home/cyrille/git/visky/data/misc/clusters.channels.npy", &size);
    n_clusters = size / 8;

    // Spike times.
    spike_times = (double*)read_npy("/home/cyrille/git/visky/data/misc/spikes.times.npy", &size);
    n_spikes = size / 8;

    // Spike clusters.
    spike_clusters =
        (int32_t*)read_npy("/home/cyrille/git/visky/data/misc/spikes.clusters.npy", &size);

    // Spike amps.
    spike_amps = (double*)read_npy("/home/cyrille/git/visky/data/misc/spikes.amps.npy", &size);

    vec3* cluster_positions = calloc(n_clusters, sizeof(vec3));
    int64_t ch = 0;
    for (uint32_t i = 0; i < n_clusters; i++)
    {
        ch = cluster_channels[i];
        glm_vec3_copy(channel_positions[ch], cluster_positions[i]);
        cluster_positions[i][0] += .05 * randn();
        cluster_positions[i][1] += .05 * randn();
        cluster_positions[i][2] += .05 * randn();
    }

    uint32_t N = n_clusters;

    // Visual data.
    vec3* position = calloc(N, sizeof(vec3));
    VkyColor* color = calloc(N, sizeof(VkyColor));
    point_size = calloc(N, sizeof(float));
    float* half_life = calloc(N, sizeof(float));
    last_active = calloc(N, sizeof(float));

    for (uint32_t i = 0; i < N; i++)
    {
        ch = cluster_channels[i];
        ASSERT(ch < n_channels);

        glm_vec3_copy(cluster_positions[i], position[i]);

        color[i].rgb[0] = channel_colors[ch][0];
        color[i].rgb[1] = channel_colors[ch][1];
        color[i].rgb[2] = channel_colors[ch][2];
        color[i].alpha = 255;

        point_size[i] = 30; // TODO: spikes.amps
        half_life[i] = .1;
        last_active[i] = -1; // 10 * t;

        // DEBUG: random data
        // position[i][0] = .5 * randn();
        // position[i][1] = .5 * randn();
        // position[i][2] = .5 * randn();
        // color[i] = vky_color(VKY_CMAP_HSV, rand_float(), 0, 1, 1);
        // point_size[i] = 30 + 20 * randn();
        // half_life[i] = .25;
        // last_active[i] = (float)i / N * 60;
    }

    vky_visual_data_set_size(visual, N, 0, NULL, NULL);
    vky_visual_data(visual, VKY_VISUAL_PROP_POS_GPU, 0, N, position);
    vky_visual_data(visual, VKY_VISUAL_PROP_COLOR, 0, N, color);
    vky_visual_data(visual, VKY_VISUAL_PROP_SIZE, 0, N, point_size);
    vky_visual_data(visual, VKY_VISUAL_PROP_LENGTH, 0, N, half_life);
    vky_visual_data(visual, VKY_VISUAL_PROP_TIME, 0, N, last_active);

    free(half_life);
    free(color);
    free(position);

    vky_add_frame_callback(canvas, frame_callback, NULL);

    vky_run_app(app);
    vky_destroy_app(app);

    free(point_size);
    free(last_active);
    return 0;
}
