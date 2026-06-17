# 3D Mesh with Arcball Navigation

Render a lit 3D mesh and navigate it interactively with mouse-driven arcball rotation.

## Overview

The mesh visual renders indexed triangle geometry with per-vertex normals and a configurable
material. Pairing it with the arcball controller lets the user rotate the object freely in 3D with
click-and-drag. A perspective camera and a Phong material are set up explicitly so that shading
responds to the view angle as the user rotates.

## Example

=== "C"

    ```c
    #include <stdint.h>
    #include "datoviz/geom.h"
    #include "datoviz/scene.h"
    #include "datoviz/scene/arcball.h"
    #include "datoviz/scene/camera.h"

    int main(void) {
        /* scene */
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);

        /* perspective camera */
        DvzCameraDesc cam = dvz_camera_desc();
        cam.eye[0] = 0.0f; cam.eye[1] = 1.0f; cam.eye[2] = 3.0f;
        cam.fov_y = 0.7f;
        dvz_panel_set_camera(panel, &cam);

        /* sphere geometry with normals */
        DvzGeometrySphereDesc sd = dvz_geometry_sphere_desc();
        sd.radius = 1.0f;
        sd.stacks = 32;
        sd.slices = 32;
        DvzGeometry* geom = dvz_geom_sphere(&sd);
        dvz_geometry_compute_normals(geom);

        /* mesh visual */
        DvzVisual* mesh = dvz_mesh(scene, 0);
        dvz_mesh_set_geometry(mesh, geom);

        /* Phong material */
        DvzMaterialDesc mat = dvz_phong_material_desc();
        mat.phong.ambient   = 0.2f;
        mat.phong.diffuse   = 0.8f;
        mat.phong.specular  = 0.4f;
        mat.phong.shininess = 32.0f;
        dvz_visual_set_material(mesh, &mat);

        dvz_panel_add_visual(panel, mesh, NULL);

        /* arcball controller for 3D rotation */
        DvzController* ctrl = dvz_arcball(scene, NULL);
        dvz_panel_bind_controller(panel, ctrl, DVZ_DIM_MASK_XYZ);

        /* run */
        DvzApp* app = dvz_app(scene);
        dvz_view_glfw(app, figure, 800, 600, "Mesh arcball");
        dvz_app_run(app, 0);

        dvz_geometry_destroy(geom);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

**Camera.** `dvz_camera_desc()` returns a default perspective camera descriptor. Adjust `eye` to
position the viewpoint and `fov_y` (in radians) to control the field of view. Pass the descriptor
to `dvz_panel_set_camera()` before adding the controller.

**Geometry.** `dvz_geom_sphere()` generates an indexed sphere mesh from a descriptor. Call
`dvz_geometry_compute_normals()` so the shader receives per-vertex normals for shading. Other
built-in generators (`dvz_geom_cube()`, `dvz_geom_torus()`, etc.) follow the same pattern.

**Mesh visual.** `dvz_mesh()` creates the visual and `dvz_mesh_set_geometry()` uploads the indexed
triangle data. The geometry object can be destroyed after upload; the visual retains its own copy of
the data on the GPU.

**Material.** `dvz_phong_material_desc()` returns Phong material defaults. Tune `ambient`,
`diffuse`, `specular`, and `shininess` to control the shading response. Pass the filled descriptor
to `dvz_visual_set_material()` before the panel starts rendering.

**Arcball controller.** `dvz_arcball()` creates the controller and
`dvz_panel_bind_controller(..., DVZ_DIM_MASK_XYZ)` attaches it to all three axes. The user can then
left-click-drag to rotate and right-click-drag or scroll to zoom.

## Common patterns

**Load geometry from an OBJ file.**

```c
/* dvz_geom_obj() reads positions, normals, and indices from a Wavefront OBJ */
DvzGeometry* geom = dvz_geom_obj("model.obj");
```

**Standard (PBR-like) material.**

```c
DvzMaterialDesc mat = dvz_standard_material_desc();
mat.standard.roughness   = 0.4f;
mat.standard.specular    = 0.5f;
mat.standard.rim_strength = 0.2f;
dvz_visual_set_material(mesh, &mat);
```

**Offscreen render.**

```c
DvzApp* app = dvz_app(scene);
DvzView* view = dvz_view_offscreen(app, figure, 800, 600);
dvz_app_run(app, 1);
dvz_view_capture_png(view, "mesh.png");
```

## See also

- [Lighting and materials](lighting-and-materials.md)
- [3D navigation](3d-navigation.md)
- [Rendering techniques](rendering-techniques.md)
- [Render offscreen](render-offscreen.md)
