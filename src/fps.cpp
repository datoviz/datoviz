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

static void compute_hist(uint32_t count, double* values, dvec2 min_max, uint32_t bins, float* hist)
{
    ANN(values);
    ANN(hist);
    if (count <= 2)
        return;

    memset(hist, 0, bins * sizeof(float));

    if (min_max[0] == 0 && min_max[1] == 0)
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
        ASSERT((int)bin < (int)bins);

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

    double interval = dvz_clock_interval(&fps->clock);

    uint64_t counter_mod = fps->counter % DVZ_FPS_MAX_COUNT;
    ASSERT(counter_mod < DVZ_FPS_MAX_COUNT);
    fps->values[counter_mod] = 1. / interval;

    fps->count = MIN(fps->counter, DVZ_FPS_MAX_COUNT);

    dvz_clock_tick(&fps->clock);
    fps->counter++;
}



void dvz_fps_histogram(DvzFps* fps)
{
    ANN(fps);

    // Compute the min and max of the values.
    dvec2 min_max = {0};
    dvz_range(fps->count, fps->values, min_max);

    // Keep the absolute maximum value.
    fps->min_max[1] = MAX(fps->min_max[1], min_max[1]);

    // Compute the FPS histogram.
    compute_hist(fps->count, fps->values, fps->min_max, DVZ_FPS_BINS, fps->hist);

    // Compute the average FPS.
    double mean = fps->count >= 2 ? dvz_mean(fps->count, fps->values) : 1.0;

    // Generate the FPS string.
    char str[32] = {0};
    snprintf(str, 32, "FPS: %04.0f/s", mean);

    // Use the full line width.
    ImGui::PushItemWidth(-1);
    // No background color.
    ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
    // Histogram color.
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, (ImVec4)ImColor::HSV(0, 0, 0.5f));

    // Histogram.
    ImGui::PlotHistogram(
        "", fps->hist, DVZ_FPS_BINS, 0, str, FLT_MAX, FLT_MAX, ImVec2(0, DVZ_FPS_HEIGHT));

    ImGui::PopStyleColor(2);
    ImGui::PopItemWidth();
}



void dvz_fps_destroy(DvzFps* fps)
{
    ANN(fps);
    FREE(fps->values);
    FREE(fps->hist);
}
