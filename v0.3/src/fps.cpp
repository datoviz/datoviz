/*************************************************************************************************/
/*  FPS widget                                                                                   */
/*************************************************************************************************/

#include "fps.h"
#include "datoviz_math.h"
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
    double min = min_max[0];
    double max = min_max[1];
    double diff = min < max ? max - min : 1;

    double bin = 0, value = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        // Normalize in [0, 1].
        value = values[i];
        bin = (value - min) / diff;
        ASSERT((0 <= bin) && (bin <= 1));

        // Normalize in [0, bins-1].
        bin = round(bin * bins);
        bin = CLIP(bin, 0, bins - 1);
        ASSERT((0 <= bin) && (bin <= bins - 1));
        ASSERT((int)bin < (int)bins);

        hist[(int)bin]++;
    }
}



static double compute_fps(uint64_t counter, uint32_t count, double* values)
{
    if (count == 0)
        return 0;

    ASSERT(count > 0);
    ANN(values);

    uint64_t counter_mod = counter % DVZ_FPS_MAX_COUNT;
    ASSERT(counter_mod < DVZ_FPS_MAX_COUNT);

    double mean = 0;
    double sum = 0;
    uint32_t idx = 0;
    uint32_t k = 0;
    for (int32_t i = (int32_t)count; i >= 0; i--)
    {
        idx = (uint32_t)i % DVZ_FPS_MAX_COUNT;
        ASSERT(idx < DVZ_FPS_MAX_COUNT);
        sum += values[idx];
        if (sum > DVZ_FPS_CUTOFF)
        {
            break;
        }
        k++;
    }
    if (k == 0)
        return 0;
    ASSERT(k > 0);
    ASSERT(sum > 0);

    return k / sum;
}



static void adaptive_frame_pacing(DvzFps* fps, double interval)
{
    if (fps->target_fps <= 0)
        return;

    // Target time per frame (seconds)
    double target = fps->target_frame_time;

    // Compute error: positive if frame was too fast
    double error = target - interval;

    // Integrate the error over time
    fps->error_integral += error;

    // Proportional-Integral control to adjust sleep time
    fps->sleep_time += fps->gain_p * error + fps->gain_i * fps->error_integral;

    // Clamp to non-negative sleep time
    if (fps->sleep_time < 0.0)
    {
        fps->sleep_time = 0.0;
        fps->error_integral = 0.0; // Optional: reset integral when overshooting too far
    }

    // Sleep for the computed duration
    if (fps->sleep_time > 0.0)
    {
        int sleep_us = (int)(fps->sleep_time * 1000000.0);
        if (sleep_us > 0)
            dvz_sleep_us(sleep_us);
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



void dvz_fps_target(DvzFps* fps, double target_fps, double gain_p, double gain_i)
{
    ANN(fps);
    log_debug("set adaptive FPS target: %d FPS", target_fps);
    fps->target_fps = target_fps;
    fps->target_frame_time = (target_fps > 0) ? 1.0 / target_fps : 0.0;
    fps->gain_p = gain_p;
    fps->gain_i = gain_i;
    fps->sleep_time = 0.0;
    fps->error_integral = 0.0;
}



void dvz_fps_tick(DvzFps* fps)
{
    ANN(fps);
    ANN(fps->values);

    double interval = dvz_clock_interval(&fps->clock);

    // HACK: avoid the initial large interval value corresponding to the first ticks
    if (fps->counter <= 20)
        interval = 0.001;

    uint64_t counter_mod = fps->counter % DVZ_FPS_MAX_COUNT;
    ASSERT(counter_mod < DVZ_FPS_MAX_COUNT);
    double value = interval; // > 0 ? 1. / interval : 0;
    fps->values[counter_mod] = value;

    fps->count = MIN(fps->counter, DVZ_FPS_MAX_COUNT);
    // fps->max = MAX(fps->max, value);

    dvz_clock_tick(&fps->clock);
    fps->counter++;

    adaptive_frame_pacing(fps, interval);
}



void dvz_fps_histogram(DvzFps* fps)
{
    ANN(fps);

    // Compute the min and max of the values.
    dvec2 min_max = {0, -1000000};
    dvz_range(fps->count, fps->values, min_max);

    // Keep the absolute maximum value.
    // fps->min_max[1] = MAX(fps->min_max[1], min_max[1]);

    // Compute the FPS histogram.
    compute_hist(fps->count, fps->values, min_max, DVZ_FPS_BINS, fps->hist);

    // Compute the average FPS.
    double mean = compute_fps(fps->counter, fps->count, fps->values);

    // Generate the FPS string.
    char str[32] = {0};
    snprintf(str, 32, "FPS: %04.0f/s", mean);

    // Use the full line width.
    ImGui::PushItemWidth(-1);
    // No background color.
    ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
    // Histogram color.
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, (ImVec4)ImColor::HSV(0, 0, 0.75f));

    // Histogram.
    ImGui::PlotHistogram(
        "##FPS Histogram", fps->hist, DVZ_FPS_BINS, 0, str, FLT_MAX, FLT_MAX,
        ImVec2(0, DVZ_FPS_HEIGHT));

    ImGui::PopStyleColor(2);
    ImGui::PopItemWidth();
}



void dvz_fps_destroy(DvzFps* fps)
{
    ANN(fps);
    FREE(fps->values);
    FREE(fps->hist);
}
