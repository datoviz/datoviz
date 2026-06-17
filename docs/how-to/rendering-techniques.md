# Rendering Techniques

Enable post-processing effects and rendering quality options on a panel or visual.

## Overview

Datoviz provides several rendering techniques that can be layered onto any panel or visual: Eye-Dome Lighting (EDL) for point-cloud depth perception, SSAO for ambient occlusion, MSAA for anti-aliasing, depth cueing for distance fade, and alpha blending for transparency. Each is opt-in and independent.

## Example

=== "C"

    ```c
    #include <stdint.h>
    #include <stdlib.h>
    #include "datoviz/scene.h"

    #define N 5000

    int main(void) {
        /* data: random 3D point cloud */
        float pos[N * 3];
        uint8_t color[N * 4];
        float size[N];
        for (int i = 0; i < N; i++) {
            pos[3*i+0] = (float)rand() / RAND_MAX * 2 - 1;
            pos[3*i+1] = (float)rand() / RAND_MAX * 2 - 1;
            pos[3*i+2] = (float)rand() / RAND_MAX * 2 - 1;
            color[4*i+0] = 100; color[4*i+1] = 180; color[4*i+2] = 220; color[4*i+3] = 200;
            size[i] = 6.0f;
        }

        /* scene */
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);
        DvzController* controller = dvz_arcball(scene, NULL);
        dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ);

        /* visual */
        DvzVisual* visual = dvz_point(scene, 0);
        dvz_visual_set_data(visual, "position", pos, N);
        dvz_visual_set_data(visual, "color", color, N);
        dvz_visual_set_data(visual, "size", size, N);
        dvz_panel_add_visual(panel, visual, NULL);

        /* enable Eye-Dome Lighting for depth perception */
        DvzEdlDesc edl = dvz_edl_desc();
        edl.enabled = true;
        dvz_panel_set_edl(panel, &edl);

        DvzApp* app = dvz_app(scene);
        dvz_view_glfw(app, figure, 800, 600, "EDL point cloud");
        dvz_app_run(app, 0);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

Create a scene, figure, panel, and visual as usual. The visual here is a 3D point cloud with an arcball controller so the depth effect is visible from any angle.

Call `dvz_edl_desc()` to get a descriptor initialised with default values, set `edl.enabled = true`, then pass it to `dvz_panel_set_edl(panel, &edl)`. EDL sharpens the apparent depth of dense point clouds by darkening edges based on normal discontinuities — no geometry changes required.

To disable a technique at runtime, pass `NULL` as the descriptor pointer (e.g. `dvz_panel_set_edl(panel, NULL)`).

## Variants

### MSAA (anti-aliasing)

```c
/* enable 8× multi-sample anti-aliasing */
DvzMsaaDesc msaa = dvz_msaa_desc();
msaa.enabled = true;
msaa.sample_count = 8u;
msaa.alpha_to_coverage = false;
dvz_panel_set_msaa(panel, &msaa);
```

### SSAO (ambient occlusion)

```c
/* enable screen-space ambient occlusion */
DvzSsaoDesc ssao = dvz_ssao_desc();
ssao.enabled = true;
dvz_panel_set_ssao(panel, &ssao);
```

### Depth cueing

Depth cueing fades distant points toward a background color, useful for large 3D scatter plots.

```c
/* fade points by depth */
DvzDepthCueDesc cue = dvz_depth_cue_desc();
cue.mode = DVZ_DEPTH_CUE_MODE_FADE;
cue.metric = DVZ_DEPTH_CUE_METRIC_DEPTH;
cue.falloff = DVZ_DEPTH_CUE_FALLOFF_LINEAR;
dvz_visual_set_depth_cue(visual, &cue);
```

### Alpha blending (transparency)

```c
/* enable source-over transparency for a visual */
dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_BLENDED);
```

## See also

- [Lighting and materials](lighting-and-materials.md) — per-mesh shading and textures
- [3D navigation](3d-navigation.md) — arcball, turntable, fly controllers
- [Add a visual](add-a-visual.md) — creating and configuring visuals
