# Handoff: user_scale style-unit architecture

Date: 2026-06-23
Branch: `v0.4-dev`

## Repository Rules

- Follow root `AGENTS.md`.
- Do not stage or commit `data` submodule changes unless explicitly approved in the current turn.
- Do not stage generated/runtime binary payloads such as `libs/vulkan/`, `*.dylib`, `*.so`,
  `*.dll`, `*.npy`, `*.npz`, or `.DS_Store`.
- Always run `git diff --check` before finalizing code changes.
- Before committing, run `git status --short` and `git diff --cached --stat`; verify the staged set
  excludes `data`, generated files, vendored runtime libraries, large binaries, and unrelated user
  changes.

## User Report

The `examples/c/features/user_scale.c` example shows marker edges that do not scale consistently
with marker size when `user_scale` changes.

Current narrow diagnosis:

- marker `diameter_px` is stored as the visual storage attr `size`;
- `size` uploads are already treated as screen-space style payloads and lowered by
  `_scene_frame_plan_upload_style_bytes()`;
- built-in marker edge width is stored in the shared point-style material payload, currently
  `DvzSceneMaterialParams.params[0]`;
- point lowering marks that material payload as screen-scaled;
- marker lowering requests material params for built-in symbols but does not mark the point-style
  material payload as screen-scaled.

The one-line local fix would be to set `out->material_params_screen_scaled = true` for marker
lowering when the built-in marker path uses point-style material params. That is likely correct for
the immediate bug, but it should not be the long-term architecture.

## Long-Term Architecture

The durable architecture should make units explicit at scene emission boundaries.

Scene-retained state should store authored values, not physical values:

- `diameter_px`, `pixel_size_px`, `stroke_width_px`, marker edge width, tick lengths, label gaps, text sizes,
  margins, hit slop, and similar UI-style quantities are authored in logical pixels unless the API
  explicitly says otherwise;
- data/world quantities remain in domain units and are never multiplied by `user_scale`;
- `device_scale` and `user_scale` are view/figure realization inputs, not persistent mutations of
  scene-owned visual data;
- `render_scale` controls render-target/output resolution and should not silently change authored
  UI style semantics.

Frame emission should lower authored logical style values into runtime payloads:

```text
physical_style_px = logical_style_px * device_scale * user_scale
```

DRP2/runtime/shaders should receive values in the units their shader math expects. For the current
point, marker, pixel, segment, and path shaders, that means physical framebuffer pixels for
screen-space size and width payloads. The runtime should not need to know about `user_scale`.

The missing abstraction is a general unit contract for emitted payload fields:

```c
typedef enum DvzScenePayloadUnit
{
    DVZ_SCENE_PAYLOAD_UNIT_NONE = 0,
    DVZ_SCENE_PAYLOAD_UNIT_LOGICAL_PX,
    DVZ_SCENE_PAYLOAD_UNIT_PHYSICAL_PX,
    DVZ_SCENE_PAYLOAD_UNIT_DATA,
    DVZ_SCENE_PAYLOAD_UNIT_NORMALIZED,
} DvzScenePayloadUnit;
```

Every emitted float payload that can be affected by DPI or user scale should have unit metadata.
This includes dense visual attrs and uniform/material fields. A material uniform should not be an
opaque float bag for scale-sensitive values.

Preferred implementation direction:

1. Add implementation-facing metadata for emitted payload units.
   - Dense attrs already have family/role metadata through `DvzFramePlanResourceRole`.
   - Extend this idea to uniform/material fields instead of using
     `material_params_screen_scaled`.

2. Replace the coarse `material_params_screen_scaled` boolean with field-level metadata.
   - Minimal shape: a mask or small descriptor saying `params[0]` is `LOGICAL_PX`.
   - Better shape: typed material/style payload descriptors per shader family, each field carrying
     name, offset, type, and unit.
   - Do not rely on visual-family lowering code remembering which anonymous float slot is special.

3. Centralize style-unit lowering.
   - One lowering helper should take `figure`, source bytes, type/field metadata, and destination
     bytes.
   - It should scale only fields declared `LOGICAL_PX`.
   - It should reject unsupported element types rather than silently treating arbitrary uniform
     bytes as floats.

4. Centralize scale-driven dirty propagation by unit dependency.
   - On `device_scale` or `user_scale` changes, mark emitted resources dirty if their payload
     metadata contains `LOGICAL_PX`.
   - This should cover attrs, material/style uniforms, generated geometry caches, text/glyph
     realization, annotation layout, and query/hit-slop resources.
   - Avoid hard-coded visual-type lists such as "point, marker, path" over time.

5. Keep authored data queryable in authored units.
   - `dvz_visual_data()` should continue returning the retained authored payload, not the scaled
     upload payload.
   - Query/readout code should state whether its tolerance/result dimensions are logical pixels,
     physical pixels, or data units.

6. Make API breakage acceptable where it removes ambiguity.
   - If public C structs expose ambiguous fields such as `stroke_width_px` without a unit suffix,
     prefer renaming new v0.4-facing fields to `*_px` or documenting the logical-pixel contract
     directly in the header.
   - Do not preserve v0.3 ambiguity if it conflicts with the v0.4 architecture.

## Immediate Implementation Slice

For the next pass, fix the marker-edge bug in the smallest way that remains compatible with the
above architecture:

1. In `src/scene/visuals/marker/lowering.c`, mark built-in marker point-style material params as
   screen-scaled.
2. Add a focused regression test proving marker edge `stroke_width_px` upload is multiplied by
   `user_scale`.
3. Keep the test framed as the current implementation contract, not as the final architecture.
4. Add a TODO near the `material_params_screen_scaled` boolean pointing at this handoff, or replace
   the boolean with a field mask if the patch remains small.

Suggested narrow validation:

```sh
just test scene_dpi
just test scene_visuals
just example-c features/user_scale
git diff --check
```

If the implementation touches shared frame-plan upload metadata, also run:

```sh
just test scene
just spec-check
```

## Code Pointers

- `spec/scene/integration/DPI_SCALE_IMPLEMENTATION_PLAN.md`
  - Defines UI-style pixel quantities and the `device_scale * user_scale` rule.
- `src/scene/core/panel_layout.c`
  - `_scene_screen_scale()` resolves the current scalar scale.
  - `_scene_figure_mark_screen_space_dirty()` currently hard-codes scale dirty propagation.
- `src/scene/scene_emit/upload_support.c`
  - `_scene_frame_plan_upload_style_bytes()` scales dense float attrs for screen-space roles.
- `src/scene/scene_emit/material_upload.c`
  - `_scene_emit_visual_material_upload()` applies the current coarse material-param scaling flag.
- `src/scene/scene_emit/visual_lowering.h`
  - Contains `material_params_screen_scaled`, the boolean that should be generalized.
- `src/scene/visuals/point/lowering.c`
  - Point style already opts into material-param screen scaling.
- `src/scene/visuals/marker/lowering.c`
  - Marker built-in path requests material params but currently does not opt into screen scaling.
- `src/scene/visuals/point/api.c`
  - `_point_style_sync_params()` stores `stroke_width_px` into `params[0]`.
- `src/scene/shaders/wgsl/marker.frag.wgsl`
  - Marker shader consumes the edge width relative to the already-scaled marker sprite size.

## Design Test To Add Later

Add a test that creates a point and a marker with the same logical diameter_px and stroke width, emits
with `user_scale = 1.0` and `user_scale = 2.0`, and asserts:

- uploaded `size` doubles;
- uploaded point style edge width doubles;
- uploaded marker style edge width doubles;
- retained `dvz_visual_data(..., "diameter_px", ...)` still returns the authored logical diameter_px;
- retained marker style state still stores the authored logical `stroke_width_px`.

That test captures the architecture better than an example screenshot because it verifies the
retained-state/emitted-payload split directly.
