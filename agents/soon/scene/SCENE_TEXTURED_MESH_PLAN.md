# Scene Textured Mesh Plan

> **Execution Status**
> - **Status:** `READY FOR IMPLEMENTATION`
> - **Updated on:** `2026-05-27`
> - **Purpose:** hand off the retained textured-mesh feature-freeze blocker to a future agent.
> - **Audience:** agents implementing mesh UV texture binding in the active scene -> DRP2 runtime.

Use this file as the implementation starting point for retained textured mesh. The feature is a
v0.4 release blocker for the terrain/planet gallery proof. Keep this lane narrow and do not create
a parallel renderer, frame stream, Vulkan wrapper, or mesh renderer.


## Target Slice

Implement one explicit retained textured-mesh path:

1. mesh vertex positions and optional indices use the existing mesh path;
2. `texcoords` is a per-vertex `vec2f` attribute;
3. one scene-owned 2D `DVZ_FIELD_FORMAT_RGBA8_UNORM` sampled field is bound as the mesh texture;
4. `DVZ_MESH_COLOR_TEXTURE` selects texture color explicitly;
5. linear filtering is the default sampler;
6. if `normal` exists, use existing material lighting; otherwise render the textured mesh unlit;
7. texture field replacement, resize, and partial updates must not reupload unrelated mesh buffers.

Explicitly defer scalar mesh textures, colormaps, normal maps, roughness/metallic maps, PBR material
graphs, cubemaps, skyboxes, terrain LOD, asset download/cache automation, and mesh face picking.


## Public API Decisions

Add a small mesh color-mode enum:

```c
typedef enum
{
    DVZ_MESH_COLOR_VERTEX = 0,
    DVZ_MESH_COLOR_TEXTURE,
} DvzMeshColorMode;
```

Add:

```c
DVZ_EXPORT int dvz_mesh_set_color_mode(DvzVisual* visual, DvzMeshColorMode mode);
```

Use sampled fields for texture binding:

```c
DvzVisual* mesh = dvz_mesh(scene, 0);
dvz_mesh_set_geometry(mesh, geometry);

DvzSampledField* tex = dvz_sampled_field(scene, &(DvzSampledFieldDesc){
    .dim = DVZ_FIELD_DIM_2D,
    .format = DVZ_FIELD_FORMAT_RGBA8_UNORM,
    .semantic = DVZ_FIELD_SEMANTIC_COLOR,
    .width = width,
    .height = height,
    .depth = 1,
});
dvz_sampled_field_set_data(tex, &view);

dvz_visual_set_field(mesh, "texture", tex);
dvz_mesh_set_color_mode(mesh, DVZ_MESH_COLOR_TEXTURE);
```

Do not add a `dvz_mesh_set_texture()` convenience wrapper in the first slice. Reusing
`DvzSampledField` gives ownership, dirty-region updates, resize behavior, and future sharing.

Do not auto-enable texture mode when a texture is bound. Make the caller choose texture color
explicitly so missing color attributes do not silently change rendering behavior.


## Implementation Steps

1. Public headers:
   - add `DvzMeshColorMode` to `include/datoviz/scene/enums.h`;
   - add `dvz_mesh_set_color_mode()` to `include/datoviz/scene.h`;
   - update the mesh docs to mention `texcoords`, `"texture"` sampled-field binding, and explicit
     texture color mode.

2. Retained scene state:
   - store mesh color mode on `DvzVisual`;
   - generalize `visual->field` / `visual->texture` comments so mesh can use them as texture input;
   - extend `dvz_visual_set_field()` to accept mesh visuals with slot `"texture"`, requiring a
     same-scene 2D RGBA8 field;
   - make sampled-field destroy, dirty, resize, and partial-update propagation include mesh texture
     bindings.

3. Frame-plan uploads:
   - extend `src/scene/visual_uploads.c` to emit mesh texture uploads through
     `_scene_emit_sampled_field_texture_upload()`;
   - in texture mode, require non-empty `texcoords` and a bound texture field;
   - in texture mode, do not require vertex `color`;
   - keep existing vertex-color mesh behavior unchanged.

4. Typed metadata and descriptor lowering:
   - add a textured-mesh visual descriptor kind in `src/scene/_visual_pipeline.h`;
   - extend `src/scene/visual_metadata.c` to include `texcoords_id`, `texture_id`, and material id
     for textured mesh;
   - extend `src/scene/visual_desc.c` so mesh + texture mode lowers to textured mesh, not generic
     primitive.

5. Bind-group ABI:
   - add mesh texture/material binding constants in `src/scene/_scene_shader_abi.h`;
   - use one dedicated set-1 layout for textured mesh:
     - binding 0: material params uniform;
     - binding 1: sampled texture;
     - binding 2: sampler;
   - extend `src/scene/visual_bind_desc.c` and `src/scene/runtime_bind_groups.c`;
   - ensure scene occlusion uses set 2 when set 1 is occupied by the textured-mesh layout.

6. Pipeline and shaders:
   - add pipeline layout variants in `src/scene/visual_pipeline_desc.c`;
   - unlit textured layout: `position`, `texcoords`;
   - lit textured layout: `position`, `texcoords`, `normal`;
   - add shader registry enum/resource entries;
   - add GLSL and WGSL shader files:
     - `mesh_textured.vert`;
     - `mesh_textured.frag`;
     - `mesh_textured_lit.vert`;
     - `mesh_textured_lit.frag`.

7. Shader semantics:
   - fragment base color is `sampled_texture_rgba * material.base_color_factor`;
   - material opacity participates in final alpha;
   - lit variant feeds the sampled base color into the existing material lighting path;
   - unlit variant still supports alpha discard and scene occlusion.

8. Geometry helper:
   - keep `dvz_mesh_set_geometry()` uploading geometry UVs when available;
   - do not let `dvz_mesh_set_geometry()` enable texture mode.


## Tests And Proof

Add focused scene tests before or with the implementation:

1. textured mesh rejects texture mode without `texcoords`;
2. textured mesh rejects texture mode without a bound texture field;
3. textured mesh accepts missing vertex `color`;
4. texture field upload emits texture creation/write commands;
5. texture field update emits a texture write without unrelated mesh-buffer uploads;
6. indexed textured mesh draw uses position/uv/normal vertex inputs;
7. textured mesh bind group layout has material + texture + sampler;
8. scene occlusion still binds correctly when textured mesh occupies set 1.

Add a deterministic C proof example using generated pixels, not binary assets. Prefer a terrain or
planet-surface mesh with UVs and one capture/smoke path.


## Validation

Run the narrow validation loop from the repository root:

```bash
git diff --check
just build
just test scene
just spec-check
```

For runtime or synchronization edits, also run a bounded offscreen or GLFW smoke with Vulkan
validation layers when available.
