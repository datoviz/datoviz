"""Execute the presentation clock and showcase callbacks without a graphics device."""

from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
SHOWCASES = ROOT / "examples/c/showcases"


def _function_source(path: Path, signature: str) -> str:
    """Read an actual C function, retaining its production expressions."""
    source = path.read_text(encoding="utf-8")
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def _frame_body(name: str) -> str:
    """Read the actual scenario callback under a unique harness name."""
    return _function_source(
        SHOWCASES / f"{name}.c", "static void _scenario_frame("
    ).replace("_scenario_frame(", f"{name}_frame(", 1)


class HomepagePresentationMotionTests(unittest.TestCase):
    def test_actual_clocks_and_showcase_callbacks(self) -> None:
        """Presentation changes leave scientific samples and registered layers intact."""
        compiler = shutil.which("cc")
        if compiler is None:
            self.skipTest("C compiler unavailable")
        cortex = (SHOWCASES / "cortical_activity.c").read_text(encoding="utf-8")
        constants = "\n".join(
            line for line in cortex.splitlines()
            if re.match(r"#define (ROTATION_|ACTIVITY_LOOP_)", line)
        )
        callbacks = "\n\n".join(
            _frame_body(name)
            for name in ("protein", "point_cloud", "cortical_activity", "textured_planet")
        )
        activation = _function_source(
            ROOT / "examples/c/runner/scenario_runner.c", "static bool _scenario_frame_active("
        )
        harness = r'''
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <cglm/affine.h>
#include "datoviz/scene.h"
#include "runner/scenario_runner.h"
#include "showcases/textured_planet_orbits.h"
#define GLOBE_VISUAL_COUNT 4
static const float TAU = 6.28318530718f;
static mat4 transforms[16];
static double orbital_time;
static float camera_yaw;
typedef struct {
    DvzVisual* presentation_visuals[3];
    vec3 presentation_axis;
} ProteinState;
typedef struct {
    DvzCamera* camera;
    DvzFly* fly;
    DvzCameraView base_view;
} PointCloudState;
typedef struct {
    struct { uint32_t time_count; float* times_ms; } data;
    DvzArcball* arcball;
    DvzVisual* mesh;
    double rotation_time;
    float peak_time_ms, current_time_ms, playback_speed;
    bool playing, loop;
} CorticalActivityState;
typedef struct {
    DvzVisual *visual, *orbit_glow_visual, *orbit_visual, *debris_visual;
    TexturedPlanetOrbitModel orbit_model;
    vec3* debris_positions;
    double debris_time;
    float debris_speed;
    bool animate_debris;
} TexturedPlanetState;
DvzResult dvz_visual_set_transform(DvzVisual* visual, mat4 transform) {
    glm_mat4_copy(transform, transforms[(uintptr_t)visual]); return DVZ_OK;
}
DvzResult dvz_visual_set_data(DvzVisual* visual, const char* attr, const void* data,
                            uint32_t count) {
    (void)visual; (void)attr; (void)data; (void)count; return DVZ_OK;
}
DvzResult dvz_fly_set_camera(DvzFly* fly, DvzCamera* camera) {
    (void)fly; (void)camera; return DVZ_OK;
}
DvzResult dvz_fly_reset(DvzFly* fly) { (void)fly; return DVZ_OK; }
DvzResult dvz_fly_pivot(DvzFly* fly, vec3 pivot) {
    (void)fly; (void)pivot; return DVZ_OK;
}
DvzResult dvz_fly_orbit(DvzFly* fly, float yaw, float pitch) {
    (void)fly; (void)pitch; camera_yaw = yaw; return DVZ_OK;
}
bool dvz_arcball_is_interacting(DvzArcball* arcball) { (void)arcball; return false; }
static bool _set_activity_time(CorticalActivityState* state, float time_ms) {
    state->current_time_ms = time_ms; return true;
}
void textured_planet_orbit_model_positions(const TexturedPlanetOrbitModel* model,
                                          double time_s, float (*positions)[3]) {
    (void)model; (void)positions; orbital_time = time_s;
}
'''
        harness += constants + "\n" + callbacks + "\n" + activation + r'''
static void near(double actual, double expected) { assert(fabs(actual - expected) < 1e-6); }
int main(void) {
    DvzScenarioSpec spec = {0};
    assert(!_scenario_frame_active(&spec, false));
    assert(!_scenario_frame_active(&spec, true));
    spec.frame = protein_frame;
    assert(_scenario_frame_active(&spec, false));
    assert(_scenario_frame_active(&spec, true));
    spec.frame_preview_only = true;
    assert(!_scenario_frame_active(&spec, false));
    assert(_scenario_frame_active(&spec, true));
    DvzScenarioContext ctx = {0};
    ctx.preview_mode = true;
    ctx.preview_frame_count = 864;
    ctx.preview_sample_stride = 1;
    ctx.preview_fps = 60;
    ctx.preview_time_scale = 8;
    ctx.preview_presentation = DVZ_SCENARIO_PRESENTATION_SWAY;
    ctx.preview_presentation_amplitude_deg = 12;
    ctx.preview_presentation_period = 14.4;
    const double amplitude = 12 * (6.283185307179586 / 360);
    for (uint64_t frame = 0; frame < 864; frame++) {
        ctx.preview_frame_index = frame;
        const double expected = amplitude * sin(6.283185307179586 * frame / 864.0);
        near(scenario_preview_presentation_angle(&ctx), expected);
        near(dvz_scenario_preview_time(&ctx), frame * 8.0 / 60);
    }
    ctx.preview_frame_index = 216;
    near(scenario_preview_presentation_angle(&ctx), amplitude);
    ctx.preview_time_scale = 1;
    near(scenario_preview_presentation_angle(&ctx), amplitude);
    near(dvz_scenario_preview_time(&ctx), 3.6);
    ctx.preview_sample_stride = 3;
    ctx.preview_frame_index = 216 * 3 + 2;
    near(scenario_preview_presentation_angle(&ctx), amplitude);
    near(dvz_scenario_preview_time(&ctx), (216 * 3 + 2) / 60.0);
    ctx.preview_sample_stride = 1;
    ctx.preview_frame_index = 863;
    const double last = scenario_preview_presentation_angle(&ctx);
    assert(last < 0 && last != 0);
    ctx.preview_frame_index = 864;
    near(scenario_preview_presentation_angle(&ctx), 0);
    ctx.preview_frame_index = 1;
    near(scenario_preview_presentation_angle(&ctx), -last);

    ctx.preview_frame_index = 216;
    ProteinState protein = {0};
    protein.presentation_axis[1] = 1;
    for (uintptr_t i = 0; i < 3; i++) protein.presentation_visuals[i] = (DvzVisual*)(i + 1);
    protein_frame(&ctx, &protein);
    assert(memcmp(transforms[1], transforms[2], sizeof(mat4)) == 0);
    assert(memcmp(transforms[1], transforms[3], sizeof(mat4)) == 0);
    near(transforms[1][0][0], cos(amplitude));
    PointCloudState cloud = {0};
    cloud.camera = (DvzCamera*)1;
    cloud.fly = (DvzFly*)1;
    point_cloud_frame(&ctx, &cloud);
    near(camera_yaw, amplitude);

    float times[] = {0, 360};
    CorticalActivityState cortex = {0};
    cortex.data.time_count = 2;
    cortex.data.times_ms = times;
    cortex.mesh = (DvzVisual*)4;
    cortex.peak_time_ms = 106;
    TexturedPlanetState planet = {0};
    planet.visual = (DvzVisual*)5;
    planet.orbit_glow_visual = (DvzVisual*)6;
    planet.orbit_visual = (DvzVisual*)7;
    planet.debris_visual = (DvzVisual*)8;
    planet.debris_speed = 60;
    for (uint64_t frame = 0; frame < 864; frame++) {
        ctx.preview_frame_index = frame;
        ctx.preview_time_scale = 8;
        float activity_sample = 0;
        double debris_sample = 0;
        for (int mode = 0; mode < 3; mode++) {
            ctx.preview_presentation = (DvzScenarioPresentationKind)mode;
            cortical_activity_frame(&ctx, &cortex);
            textured_planet_frame(&ctx, &planet);
            if (mode == 0) {
                activity_sample = cortex.current_time_ms;
                debris_sample = orbital_time;
            } else {
                near(cortex.current_time_ms, activity_sample);
                near(orbital_time, debris_sample);
                assert(memcmp(transforms[5], transforms[6], sizeof(mat4)) == 0);
                assert(memcmp(transforms[5], transforms[7], sizeof(mat4)) == 0);
                assert(memcmp(transforms[5], transforms[8], sizeof(mat4)) == 0);
            }
            if (mode == 1) {
                near(transforms[4][0][0], 1);
                near(transforms[5][0][0], 1);
            }
        }
    }
    ctx.preview_presentation = DVZ_SCENARIO_PRESENTATION_SWAY;
    ctx.preview_mode = false;
    assert(!scenario_preview_presentation_active(&ctx));
    return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix="datoviz-presentation-test-") as directory:
            source = Path(directory) / "presentation.c"
            binary = Path(directory) / "presentation"
            source.write_text(harness, encoding="utf-8")
            subprocess.run(
                [compiler, "-std=gnu11", "-Wall", "-Wextra", "-Werror",
                 "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
                 "-Iinclude", "-Iexamples/c", "-Iexternal/cglm/include", str(source),
                 "examples/c/runner/scenario_helpers.c", "-lm", "-o", str(binary)],
                cwd=ROOT, check=True, capture_output=True, text=True,
            )
            subprocess.run([str(binary)], check=True, capture_output=True, text=True)


if __name__ == "__main__":
    unittest.main()
