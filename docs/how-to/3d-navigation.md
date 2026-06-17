# 3D Navigation

Attach a 3D navigation controller to a panel so users can rotate, orbit, or fly through a scene.

## Overview

Datoviz provides four 3D controllers: **arcball** (free rotation around a pivot), **turntable**
(world-up constrained orbit), **fly** (first-person camera), and **orbit camera** (orbit with
explicit target and up-vector). Each is created as a `DvzController*` and bound to a panel with
`dvz_panel_bind_controller`.

## Example

=== "C"

    ```c
    #include <stdint.h>
    #include "datoviz/app.h"
    #include "datoviz/geom.h"
    #include "datoviz/scene.h"

    int main(void) {
        /* scene and panel */
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);

        /* sphere mesh */
        DvzGeometrySphereDesc sdesc = dvz_geometry_sphere_desc();
        sdesc.rows = 32;
        sdesc.cols = 32;
        DvzGeometry* geo = dvz_geom_sphere(&sdesc);
        DvzVisual* visual = dvz_mesh(scene, 0);
        dvz_mesh_set_geometry(visual, geo);
        dvz_panel_add_visual(panel, visual, NULL);

        /* arcball controller — free 3D rotation */
        DvzController* controller = dvz_arcball(scene, NULL);
        dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ);

        /* run */
        DvzApp* app = dvz_app(scene);
        dvz_view_glfw(app, figure, "3D navigation", 0);
        dvz_app_run(app, 0);
        dvz_app_destroy(app);
        dvz_geometry_destroy(geo);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

Create the scene, figure, and panel as usual with `dvz_scene`, `dvz_figure`, and
`dvz_panel_full`. The panel holds the visuals and the controller.

Build a mesh geometry — here a UV sphere via `dvz_geom_sphere`. Create a `DvzVisual*` with
`dvz_mesh`, upload the geometry with `dvz_mesh_set_geometry`, and add the visual to the panel.

Create the arcball controller with `dvz_arcball(scene, NULL)`. The second argument is an optional
`DvzArcballDesc*` for initial orientation; pass `NULL` for defaults. Bind it to the panel with
`dvz_panel_bind_controller`, passing `DVZ_DIM_MASK_XYZ` so all three axes are interactive.

Launch the interactive window with the standard `dvz_app` / `dvz_view_glfw` / `dvz_app_run`
sequence. Clean up geometry separately from the scene since `DvzGeometry` is not owned by the
scene graph.

## Variants

**Turntable** — world-up constrained orbit; no roll, pitch clamped to avoid gimbal flip. Best for
upright object inspection.

```c
DvzController* controller = dvz_turntable(scene, NULL);
dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ);
```

**Fly** — first-person WASD + mouse-look camera. Suitable for large environments where the user
walks through the scene.

```c
DvzController* controller = dvz_fly(scene, NULL);
dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ);
```

**Orbit camera** — orbit around an explicit target point with a specified up-vector. Use when you
need programmatic control of the pivot.

```c
DvzController* controller = dvz_orbit_camera(scene, NULL);
dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ);
```

## See also

- [Create a scene](create-a-scene.md)
- [Lighting and materials](lighting-and-materials.md)
- [Rendering techniques](rendering-techniques.md)
- [Panzoom](use-panzoom.md)
