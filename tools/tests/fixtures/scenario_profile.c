#include "runner/scenario_runner.h"

#include <math.h>
#include <stdio.h>

/**
 * Execute the production shared runner configuration under native and WASM compilers.
 * @return Zero on successful configuration.
 */
int main(void)
{
    DvzScenarioContext native = {0};
    if (scenario_presentation_configure(&native, "showcases_cortical_activity", -1, 0, 0) != 0)
        return 1;
    native.preview_mode = true;
    native.preview_frame_index = 216;
    native.preview_fps = 60;
    native.preview_sample_stride = 1;
    const double quarter = scenario_preview_presentation_angle(&native);
    native.preview_mode = false;
    native.time = 3.6;
    const double live = scenario_preview_presentation_angle(&native);
    native.time = 123;
    const int invalid = scenario_presentation_configure(&native, NULL, 2, NAN, 0);
    DvzScenarioContext other = {0};
    scenario_presentation_configure(&other, "features_controller_arcball", -1, 0, 0);
    printf("{\"amplitude\":%.12g,\"period\":%.12g,\"quarter\":%.12g,"
           "\"live\":%.12g,\"scientific_time\":%.12g,\"invalid\":%d,\"default\":%d}\n",
           native.preview_presentation_amplitude_deg, native.preview_presentation_period,
           quarter, live, native.time, invalid, (int)other.preview_presentation);
    return 0;
}
