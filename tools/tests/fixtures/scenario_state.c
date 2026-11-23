#include "runner/scenario_runner.h"
#include "datoviz/scene.h"
#include "datoviz/scene/camera.h"
#include "datoviz/controller/arcball.h"
#include "datoviz/controller/camera.h"
#include "datoviz/input/pointer.h"
#include "core/_scene.h"

#include <stdio.h>
#include <string.h>

DvzScenarioSpec dvz_showcase_cortical_activity_scenario(void);
DvzScenarioSpec dvz_example_controller_arcball_scenario(void);

/**
 * Print a portable numeric state vector for comparison with a second backend build.
 * @param values Borrowed float values.
 * @param count Number of values.
 */
static void print_values(const float* values, unsigned count)
{
    for (unsigned i = 0; i < count; i++)
        printf(",%.9g", values[i]);
}



/**
 * Run the actual canonical scenario and controller code without a GPU runtime.
 * @param argc Argument count.
 * @param argv Optional scenario selection.
 * @return Zero when scenario initialization and state transitions succeed.
 */
int main(int argc, char** argv)
{
    const bool cortical = argc < 2 || strcmp(argv[1], "arcball") != 0;
    DvzScenarioSpec spec = cortical ? dvz_showcase_cortical_activity_scenario() :
                                      dvz_example_controller_arcball_scenario();
    DvzScenarioContext ctx = {0};
    ctx.scene = dvz_scene();
    ctx.width = ctx.logical_width = ctx.framebuffer_width = 1280;
    ctx.height = ctx.logical_height = ctx.framebuffer_height = 720;
    ctx.device_scale = ctx.user_scale = ctx.render_scale = 1;
    ctx.presentation = DVZ_RUNNER_PRESENT_BROWSER;
    if (!ctx.scene || scenario_presentation_configure(&ctx, spec.id, -1, 0, 0) != 0)
        return 1;
    void* user = NULL;
    if (!spec.init(&ctx, &user) || ctx.controller_binding_count != 1)
        return 2;
    DvzPanel* panel = ctx.controller_bindings[0].panel;
    DvzArcball* arcball = dvz_controller_arcball(ctx.controller_bindings[0].controller);
    DvzArcballState state = {0};
    DvzCameraView camera = {0};
    dvz_camera_get_view(dvz_panel_camera(panel), &camera);
    dvz_arcball_state(arcball, &state);
    printf("[%.9g", state.zoom);
    print_values(state.pan, 2);
    print_values(camera.eye, 3);
    print_values(camera.up, 3);
    ctx.time = 0;
    ctx.dt = 0;
    spec.frame(&ctx, user);
    ctx.frame_index++;
    ctx.time = 3.6;
    ctx.dt = 3.6;
    spec.frame(&ctx, user);
    ctx.frame_index++;
    mat4 transform = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
    if (ctx.primary_visual)
        dvz_visual_get_transform(ctx.primary_visual, transform);
    print_values((const float*)transform, 16);
    DvzPointerEvent event = {0};
    event.type = DVZ_POINTER_EVENT_PRESS;
    event.button = DVZ_POINTER_BUTTON_LEFT;
    event.window_size[0] = 1280;
    event.window_size[1] = 720;
    event.content_scale = 1;
    dvz_arcball_pointer(arcball, &event);
    ctx.time = 5.4;
    ctx.dt = 1.8;
    spec.frame(&ctx, user);
    if (ctx.primary_visual)
        dvz_visual_get_transform(ctx.primary_visual, transform);
    print_values((const float*)transform, 16);
    event.type = DVZ_POINTER_EVENT_DRAG;
    event.content.d.press_pos[0] = 640;
    event.content.d.press_pos[1] = 360;
    event.content.d.is_press_valid = true;
    event.pos[0] = 688;
    event.pos[1] = 384;
    dvz_arcball_pointer(arcball, &event);
    event.type = DVZ_POINTER_EVENT_DRAG_STOP;
    dvz_arcball_pointer(arcball, &event);
    event.type = DVZ_POINTER_EVENT_WHEEL;
    event.content.w.dir[0] = 0;
    event.content.w.dir[1] = 1;
    dvz_arcball_pointer(arcball, &event);
    dvz_arcball_state(arcball, &state);
    printf(",%.9g", state.zoom);
    DvzMVP mvp = {0};
    dvz_arcball_mvp(arcball, &mvp);
    print_values((const float*)mvp.model, 16);
    if (ctx.primary_visual)
    {
        DvzSceneMaterialState* material = &ctx.primary_visual->material;
        const float phong[4] = {
            material->ambient, material->diffuse, material->specular, material->shininess};
        print_values(phong, 4);
        printf(",%u", panel->lights.count);
        for (uint32_t i = 0; i < panel->lights.count; i++)
        {
            DvzLightDesc* light = &panel->lights.items[i]->desc;
            print_values(light->direction, 3);
            printf(",%.9g", light->intensity);
        }
        printf(",%u", ctx.primary_visual->attr_count);
        for (uint32_t i = 0; i < ctx.primary_visual->attr_count; i++)
            printf(",%llu", (unsigned long long)ctx.primary_visual->attrs[i].item_count);
    }
    printf("]\n");
    if (spec.destroy)
        spec.destroy(&ctx, user);
    dvz_scene_destroy(ctx.scene);
    return 0;
}
