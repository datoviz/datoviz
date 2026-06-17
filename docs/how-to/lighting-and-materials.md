# Lighting and Materials

Apply Phong or standard shading to a mesh visual by setting a material descriptor on it.

## Overview

Datoviz supports two shading models: **Phong** (`dvz_phong_material_desc`) with ambient/diffuse/specular/shininess parameters, and **Standard** (`dvz_standard_material_desc`) with roughness/specular/rim-strength. Both are set via `dvz_visual_set_material`. The light direction is part of the material descriptor and is expressed as a normalized world-space vector.

## Example

=== "C"

    ```c
    #include <stdint.h>
    #include <stdlib.h>
    #include "datoviz/geom.h"
    #include "datoviz/scene.h"

    #define N_VERTICES 0  /* dvz_geom_sphere allocates internally */

    int main(void) {
        /* scene */
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);

        /* camera */
        DvzCameraDesc camera = dvz_camera_desc();
        camera.eye[2] = 3.0f;
        camera.up[1] = 1.0f;
        dvz_panel_set_camera(panel, &camera);

        /* arcball controller */
        DvzController* controller = dvz_arcball(scene, NULL);
        dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XYZ);

        /* sphere geometry */
        DvzGeometry* geom = dvz_geom_sphere(&(DvzGeometrySphereDesc){
            DVZ_STRUCT_INIT_FIELDS(DvzGeometrySphereDesc),
            .radius  = 0.8f,
            .sectors = 64,
            .rings   = 32,
            .color   = {200, 200, 220, 255},
        });

        /* mesh visual */
        DvzVisual* visual = dvz_mesh(scene, 0);
        dvz_mesh_set_geometry(visual, geom);
        dvz_geometry_destroy(geom);

        /* Phong material: mid-gloss */
        DvzMaterialDesc mat = dvz_phong_material_desc();
        mat.phong.ambient   = 0.25f;
        mat.phong.diffuse   = 0.75f;
        mat.phong.specular  = 0.35f;
        mat.phong.shininess = 32.0f;
        /* light comes from upper-right-front */
        mat.light_direction[0] = 0.4f;
        mat.light_direction[1] = 0.6f;
        mat.light_direction[2] = 0.7f;
        dvz_visual_set_material(visual, &mat);

        dvz_panel_add_visual(panel, visual, NULL);

        DvzApp* app = dvz_app(scene);
        dvz_view_glfw(app, figure, 800, 600, "Lighting");
        dvz_app_run(app, 0);

        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

`dvz_geom_sphere` allocates a sphere geometry with the requested resolution and a flat base color. The `sectors` and `rings` parameters control tessellation quality.

`dvz_mesh` creates a generic mesh visual. `dvz_mesh_set_geometry` uploads positions, normals, and UVs from the geometry object; after that call the geometry can be destroyed with `dvz_geometry_destroy`.

`dvz_phong_material_desc` returns a default-initialized Phong descriptor. The four `phong` sub-fields control classic ambient/diffuse/specular shading. `shininess` is the Phong exponent: low values (8–16) give broad soft highlights; high values (64–256) give tight mirror-like highlights. The `light_direction` field is a world-space direction vector pointing *toward* the light; it does not need to be normalized but must be non-zero.

`dvz_visual_set_material` uploads the descriptor to the GPU and activates shading for that visual.

The `DvzCameraDesc` places the eye along +Z looking toward the origin. The arcball controller lets the user rotate the mesh with the mouse.

## Common patterns / Variants

**Standard (PBR-like) shading:**

```c
DvzMaterialDesc mat = dvz_standard_material_desc();
mat.standard.roughness    = 0.4f;
mat.standard.specular     = 0.5f;
mat.standard.rim_strength = 0.2f;
dvz_visual_set_material(visual, &mat);
```

**Matte surface:**

```c
DvzMaterialDesc mat = dvz_phong_material_desc();
mat.phong.ambient   = 0.3f;
mat.phong.diffuse   = 0.9f;
mat.phong.specular  = 0.02f;
mat.phong.shininess = 4.0f;
```

**Textured mesh:** attach a sampled field to the `"texture"` slot before setting the material.

```c
dvz_visual_set_field(visual, "texture", sampled_field);
dvz_visual_set_material(visual, &mat);
```

## See also

- [3D navigation](3d-navigation.md) — arcball and other 3D controllers
- [Use sampled fields](use-sampled-fields.md) — loading GPU textures
- [Rendering techniques](rendering-techniques.md) — SSAO, depth cue, transparency
