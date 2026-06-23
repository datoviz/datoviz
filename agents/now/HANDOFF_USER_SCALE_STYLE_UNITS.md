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

## Status

Implemented on 2026-06-23.

Relevant commits:

- `215bc2d61 Scale marker edge style with user scale`
- `d2a7b59af Describe material payload units per field`
- `1410d4272 Share unit lowering for style uploads`
- `c83fff0d9 Use explicit px names for screen-space scene API`
- `938fd9ce7 Use payload units for scale dirty propagation`

The original bug was that marker built-in edge widths did not scale consistently with marker
diameter when `user_scale` changed. Marker and point material-style edge widths now declare
field-level unit metadata, and material upload lowering converts authored logical pixels to runtime
physical pixels.

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

The implemented abstraction is a general unit contract for emitted payload fields:

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

Every emitted float payload that can be affected by DPI or user scale should have unit metadata over
time. This currently covers:

- dense visual attrs through existing frame-plan resource roles;
- point and marker material style params through `DvzScenePayloadFieldDesc`;
- shared byte lowering through `_scene_payload_lower_fields()`;
- material dirty propagation through field unit dependencies instead of a hard-coded visual-family
  list.

Still-valid direction:

1. Extend field descriptors beyond material params when new uniform/style payloads carry
   scale-sensitive values.
2. Prefer typed material/style payload structs over anonymous float slots where shader contracts
   need to grow.
3. Keep scale-driven dirty propagation metadata-based. Generated geometry caches, text/glyph
   realization, annotation layout, and query/hit-slop resources should declare their dependency or
   route through existing dirty paths.

4. Keep authored data queryable in authored units.
   - `dvz_visual_data()` should continue returning the retained authored payload, not the scaled
     upload payload.
   - Query/readout code should state whether its tolerance/result dimensions are logical pixels,
     physical pixels, or data units.

5. Make API breakage acceptable where it removes ambiguity.
   - Public scene style quantities now use explicit `_px` names for the v0.4-facing API.
   - Do not preserve v0.3 ambiguity if it conflicts with the v0.4 architecture.

## Validation Run

Validation used across the implementation commits:

```sh
just build
just test scene_dpi
just test marker
just test visuals
just example-c features/user_scale
node --check examples/webgpu/demos/wasm_2d.js
node --check examples/webgpu/fixture_dashboard.js
python3 -m py_compile examples/python/raw/offscreen_point.py examples/python/raw/async_click.py examples/python/qt/hosted_pyqt.py
git diff --check
```

## Code Pointers

- `spec/scene/integration/DPI_SCALE_IMPLEMENTATION_PLAN.md`
  - Defines UI-style pixel quantities and the `device_scale * user_scale` rule.
- `src/scene/core/panel_layout.c`
  - `_scene_screen_scale()` resolves the current scalar scale.
  - `_scene_figure_mark_screen_space_dirty()` now marks material params dirty through field unit
    metadata.
- `src/scene/scene_emit/upload_support.c`
  - `_scene_frame_plan_upload_style_bytes()` scales dense float attrs for screen-space roles.
  - `_scene_payload_lower_fields()` lowers authored payload fields into runtime units.
- `src/scene/scene_emit/material_upload.c`
  - `_scene_emit_visual_material_upload()` lowers material params through field metadata.
- `src/scene/scene_emit/visual_lowering.h`
  - Defines `DvzScenePayloadUnit` and `DvzScenePayloadFieldDesc`.
- `src/scene/visuals/point/lowering.c`
  - Point style declares logical-pixel material fields.
- `src/scene/visuals/marker/lowering.c`
  - Marker built-in path declares logical-pixel material fields.
- `src/scene/visuals/point/api.c`
  - `_point_style_sync_params()` stores `stroke_width_px` into `params[0]`.
- `src/scene/shaders/wgsl/marker.frag.wgsl`
  - Marker shader consumes the edge width relative to the already-scaled marker sprite size.

## Remaining Test To Add Later

The current DPI regression covers marker material reupload after `user_scale` changes. A broader
contract test would still be useful: create a point and a marker with the same logical diameter_px
and stroke width, emit with `user_scale = 1.0` and `user_scale = 2.0`, and assert:

- uploaded `size` doubles;
- uploaded point style edge width doubles;
- uploaded marker style edge width doubles;
- retained `dvz_visual_data(..., "diameter_px", ...)` still returns the authored logical diameter_px;
- retained marker style state still stores the authored logical `stroke_width_px`.

That test captures the architecture better than an example screenshot because it verifies the
retained-state/emitted-payload split directly.
