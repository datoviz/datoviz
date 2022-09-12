/*************************************************************************************************/
/*  FPS widget                                                                                   */
/*************************************************************************************************/

#include "fps.h"
#include "_math.h"
#include "gui.h"
#include "imgui.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void compute_hist(uint32_t count, double* values, uint32_t bins, float* hist)
{
    ANN(values);
    ANN(hist);
    if (count <= 2)
        return;

    memset(hist, 0, bins * sizeof(float));

    dvec2 min_max = {0};
    dvz_range(count, values, min_max);
    double min = 0;
    double max = min_max[1];
    double diff = min < max ? max - min : 1;

    double bin = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        // Normalize in [0, 1].
        bin = (values[i] - min) / diff;
        ASSERT((0 <= bin) && (bin <= 1));

        // Normalize in [0, bins-1].
        bin = round(bin * bins);
        bin = CLIP(bin, 0, bins - 1);
        ASSERT((0 <= bin) && (bin <= bins - 1));

        hist[(int)bin]++;
    }
}



/*************************************************************************************************/
/*  FPS                                                                                          */
/*************************************************************************************************/

DvzFps dvz_fps()
{
    INIT(DvzFps, fps);
    fps.clock = dvz_clock();
    fps.values = (double*)calloc(DVZ_FPS_MAX_COUNT, sizeof(double));
    fps.hist = (float*)calloc(DVZ_FPS_BINS, sizeof(float));
    return fps;
}



void dvz_fps_tick(DvzFps* fps)
{
    ANN(fps);
    ANN(fps->values);

    uint64_t counter_mod = fps->counter % DVZ_FPS_MAX_COUNT;
    ASSERT(counter_mod < DVZ_FPS_MAX_COUNT);

    double interval = dvz_clock_interval(&fps->clock);
    fps->values[counter_mod] = 1. / interval;

    fps->count = MIN(fps->counter, DVZ_FPS_MAX_COUNT);

    dvz_clock_tick(&fps->clock);
    fps->counter++;
}



void dvz_fps_histogram(DvzFps* fps)
{
    ANN(fps);

    // Compute the FPS histogram.
    compute_hist(fps->count, fps->values, DVZ_FPS_BINS, fps->hist);

    // Compute the average FPS.
    double mean = fps->count >= 2 ? dvz_mean(fps->count, fps->values) : 1.0;

    // Generate the FPS string.
    char str[32] = {0};
    snprintf(str, 32, "FPS: %04.0f/s", mean);

    ImGui::PushItemWidth(-1);
    ImGui::PlotHistogram(
        "", fps->hist, DVZ_FPS_BINS, 0, str, FLT_MAX, FLT_MAX, ImVec2(0, DVZ_FPS_HEIGHT));
    ImGui::PopItemWidth();
}



void dvz_fps_destroy(DvzFps* fps)
{
    ANN(fps);
    FREE(fps->values);
    FREE(fps->hist);
}
