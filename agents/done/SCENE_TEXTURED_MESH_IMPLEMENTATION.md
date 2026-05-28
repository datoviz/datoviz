# Scene Textured Mesh Implementation

> **Execution Status**
> - **Status:** `IMPLEMENTED FIRST SLICE`
> - **Updated on:** `2026-05-28`
> - **Purpose:** record the retained textured-mesh slice that landed for the active scene -> DRP2
>   runtime.
> - **Audience:** agents validating, polishing, or extending mesh UV texture binding.

Use this file as the completed implementation record for the first retained textured-mesh slice.
The feature is no longer an implementation blocker. The remaining release work is fixture/gallery
proof, validation, and any concrete polish exposed by the `textured_mesh` example or future
terrain/planet showcase.


## Landed Evidence

1. Public API and comments expose mesh texture mode through `DvzMeshColorMode`,
   `dvz_mesh_set_color_mode()`, `texcoords`, and `dvz_visual_set_field(mesh, "texture", field)`.
2. `src/scene/domain/field_texture.c` accepts mesh `"texture"` bindings for same-scene 2D
   `DVZ_FIELD_FORMAT_RGBA8_UNORM` sampled fields.
3. `src/scene/visuals/mesh/lowering.c`, `pipeline.c`, and related descriptor code lower texture
   color mode to the textured-mesh descriptor, bind layout, and pipeline metadata.
4. `src/scene/runtime/render_emit_bindings.c` resolves the textured-mesh material/texture/sampler
   bind-group layout.
5. GLSL and WGSL built-ins exist for `mesh_textured`.
6. Focused scene tests cover mesh texture field binding and textured mesh scene emission.
7. `examples/c/visuals/textured_mesh.c` renders a UV sphere with an RGBA texture, material controls,
   capture support, and generated fallback texture data when optional `data` assets are absent.


## Target Slice

The implemented first slice covers one explicit retained textured-mesh path:

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


## Implementation Record

The original implementation checklist is preserved below for traceability. Items in this section
were completed by the commits that added and then polished retained textured mesh.

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
   - extend scene emission to emit mesh texture uploads through
     `_scene_emit_sampled_field_texture_upload()`;
   - in texture mode, require non-empty `texcoords` and a bound texture field;
   - in texture mode, do not require vertex `color`;
   - keep existing vertex-color mesh behavior unchanged.

4. Typed metadata and descriptor lowering:
   - add a textured-mesh visual descriptor kind;
   - include `texcoords_id`, `texture_id`, and material id for textured mesh metadata;
   - lower mesh + texture mode to textured mesh, not generic primitive.

5. Bind-group ABI:
   - add mesh texture/material binding constants in `src/scene/_scene_shader_abi.h`;
   - use one dedicated set-1 layout for textured mesh:
     - binding 0: material params uniform;
     - binding 1: sampled texture;
     - binding 2: sampler;
   - extend visual bind descriptors and runtime bind-group resolution;
   - ensure scene occlusion uses set 2 when set 1 is occupied by the textured-mesh layout.

6. Pipeline and shaders:
   - add pipeline layout variants;
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


## Tests And Proof Status

Focused scene coverage now exists for the landed slice. Keep broadening proof around release
fixtures and showcase capture:

1. textured mesh rejects texture mode without `texcoords`;
2. textured mesh rejects texture mode without a bound texture field;
3. textured mesh accepts missing vertex `color`;
4. texture field upload emits texture creation/write commands;
5. texture field update emits a texture write without unrelated mesh-buffer uploads;
6. indexed textured mesh draw uses position/uv/normal vertex inputs;
7. textured mesh bind group layout has material + texture + sampler;
8. scene occlusion still binds correctly when textured mesh occupies set 1.

The current deterministic/live proof is `examples/c/visuals/textured_mesh.c`, which uses the Earth
texture from the `data` submodule when available and generated texture data otherwise. A dedicated
`fixture_mesh_textured.c` and promoted `textured_terrain_or_planet` showcase remain release-proof
work, not core feature implementation work.


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
