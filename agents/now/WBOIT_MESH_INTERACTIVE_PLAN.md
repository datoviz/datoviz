# WBOIT Mesh Interactive Plan

> **Execution Status**
> - **Status:** `ACTIVE EXECUTION PLAN`
> - **Updated on:** `2026-05-15`
> - **Purpose:** implementation entry point for a proper interactive 3D mesh example using
>   weighted blended order-independent transparency through the scene -> DRP2 -> vklite path.

This document is the practical starting point when resuming WBOIT work.

Normative and proposal context lives in:

1. [../../spec/scene/semantics/TRANSPARENCY.md](../../spec/scene/semantics/TRANSPARENCY.md)
2. [../../spec/scene/proposals/TRANSPARENCY_WBOIT_DESIGN.md](../../spec/scene/proposals/TRANSPARENCY_WBOIT_DESIGN.md)
3. [../../spec/drp2/CAPABILITIES.md](../../spec/drp2/CAPABILITIES.md)
4. [../../spec/drp2/COMMANDS.md](../../spec/drp2/COMMANDS.md)

Low-level Vulkan/vklite proof material already exists in:

1. [../../src/vklite/tests/test_techniques.c](../../src/vklite/tests/test_techniques.c)
2. `src/vklite/tests/shaders/wboit_accum.*`
3. `src/vklite/tests/shaders/wboit_comp.*`


## Target Outcome

Add a native interactive C example, likely `examples/c/hello_mesh_wboit_glfw.c`, that renders an
arcball-controlled 3D scene with at least one transparent mesh visual. The current example uses one
WBOIT cube between opaque reference cards and includes a GUI overlay for tuning cube RGB, alpha,
ambient/diffuse strength, and light direction at runtime.

The example must exercise the active architecture:

```text
scene visual alpha mode
  -> scene FramePlan pass split
  -> explicit DRP2 multi-pass command stream
  -> vklite runtime execution
  -> canvas/app GLFW presentation
```

Do not implement WBOIT as a scene-private Vulkan shortcut. Scene decides and plans; DRP2 represents
the work explicitly; vklite executes it.


## Current Missing Pieces

Current implementation status on `2026-05-15`:

1. Public `DvzAlphaMode` storage/accessors are present.
2. `DvzCapabilitySnapshot` has the lower-level WBOIT facts used by scene diagnostics.
3. FramePlan render-pass roles exist for opaque, transparent accumulation, WBOIT resolve, and
   picking.
4. Scene panel planning splits explicit `DVZ_ALPHA_WBOIT` visuals into transparent accumulation
   render nodes and appends a WBOIT resolve node whenever WBOIT visuals are present.
5. `DVZ_ALPHA_BLENDED` remains available as the non-WBOIT source-over alpha route for visuals that do
   not need order-independent transparency.
6. Scene lowering now emits executable DRP2 WBOIT commands with scene-owned accumulation and resolve
   shaders, RGBA16F/R16F intermediate targets, additive per-target blending, and final resolve.
7. The DRP2 vklite runtime executes the multi-pass WBOIT path through borrowed app/canvas frame
   command buffers, including intermediate texture transitions and transient depth lifetime handling.
8. `examples/c/hello_mesh_wboit_glfw.c` exists and validates as a bounded GLFW smoke.

### 1. Public scene transparency API

The current public scene headers have mesh visuals, RGBA colors, primitive shading, arcball
controllers, depth-capable rendering, and visual alpha-mode storage/accessors.

The active visual-level mode is:

```c
typedef enum
{
    DVZ_ALPHA_OPAQUE = 0,
    DVZ_ALPHA_BLENDED,
    DVZ_ALPHA_WBOIT,
    DVZ_ALPHA_MASK,
} DvzAlphaMode;

DVZ_EXPORT int dvz_visual_set_alpha_mode(DvzVisual* visual, DvzAlphaMode mode);
DVZ_EXPORT DvzAlphaMode dvz_visual_alpha_mode(const DvzVisual* visual);
```

Current implementation target:

1. `DVZ_ALPHA_OPAQUE`: existing path, depth test on, depth write on.
2. `DVZ_ALPHA_BLENDED`: ordinary source-over blending on the final target.
3. `DVZ_ALPHA_WBOIT`: explicit weighted blended OIT with accumulation and resolve passes.
4. `DVZ_ALPHA_MASK`: retained API value; mask-specific shader behavior remains a later slice.

Do not infer WBOIT only from `color.a < 255`. The user should opt the visual into transparent
rendering explicitly; per-vertex or material alpha then supplies opacity.


### 2. Capability snapshot expansion

`DvzCapabilitySnapshot` now carries lower-level facts needed to derive WBOIT availability rather
than a single `supports_wboit` boolean.

Needed fields include:

1. `max_color_attachments`
2. supported render-target formats or specific booleans for `rgba16float` and `r16float`
3. support for sampling intermediate render targets
4. `supports_color_blending`
5. enough pass/texture support for accumulation and resolve

Scene behavior should be explicit:

1. if a visual requests `DVZ_ALPHA_WBOIT` and capabilities cannot realize WBOIT, emit a diagnostic;
2. if a visual requests `DVZ_ALPHA_BLENDED`, require ordinary color blending support only;
3. do not silently downgrade WBOIT to a different transparency route.


### 3. FramePlan pass roles

Current scene planning emits separate render pass roles only for explicit WBOIT visuals. Ordinary
`DVZ_ALPHA_BLENDED` visuals stay in the final-target render pass with a source-over blend pipeline.

Add enough FramePlan metadata to represent:

1. opaque render pass
2. transparent accumulation pass
3. WBOIT resolve/composite pass
4. accumulation color target, likely RGBA16F
5. accumulation weight/reveal target, likely R16F
6. final panel color target
7. shared or coordinated depth target from the opaque pass

The intended per-panel ordering is:

```text
RenderNode: opaque pass
RenderNode: transparent accumulation pass
RenderNode: WBOIT resolve pass
```

Panels with no transparent visuals do not emit accumulation or resolve nodes.

Likely files:

1. `include/datoviz/scene/enums.h`
2. `include/datoviz/scene/types.h`
3. `src/scene/_scene.h`
4. `src/scene/_frame_plan.h`
5. `src/scene/frame_plan.c`
6. `src/scene/scene_emit.c`
7. `src/scene/tests/frame_plan.c`


### 4. DRP2 multi-attachment and blend support

The DRP2 prose spec already describes multiple color attachments and per-attachment blend state, and
the current C stream/runtime path has first support for the WBOIT-shaped command stream.

Current completed pieces:

1. `BeginRenderPass` with multiple color attachments;
2. per-attachment load/store/clear semantics;
3. `CreateRenderPipeline` color target formats;
4. per-attachment blend state;
5. render target textures that can later be sampled by the resolve pass;
6. serialization and semantic validation for the above;
7. DRP2 fixtures that encode a minimal WBOIT-like accumulation + resolve sequence.

Still pending:

1. tighter validation that pipeline color target formats match render-pass attachment formats;
2. broader recording/fixture coverage for scene-emitted WBOIT streams if DVZR replay needs it.

Likely files:

1. `include/datoviz/drp2/stream.h`
2. `src/drp2/_stream.h`
3. `src/drp2/stream.c`
4. `src/drp2/serialization.c`
5. `src/drp2/semantic.c`
6. `src/drp2/pass.c`
7. `src/drp2/pipeline.c`
8. `spec/drp2/schema/`
9. `spec/drp2/fixtures/positive/`


### 5. vklite runtime execution

The vklite runtime must lower DRP2 multi-attachment/blend commands to Vulkan dynamic rendering.

Completed runtime work:

1. route scene lowering through the tested RGBA16F and R16F accumulation render targets;
2. begin render passes with two color attachments for accumulation;
3. configure additive blending separately per attachment;
4. run transparent mesh accumulation with depth test enabled and depth write disabled;
5. transition accumulation textures to shader-read layout;
6. bind accumulation textures and sampler for resolve;
7. run fullscreen triangle resolve into the panel color target;
8. keep borrowed canvas frame target and command-buffer ownership unchanged.

Use the existing WBOIT vklite test as the behavior reference, but route the real scene path through
DRP2 rather than directly through vklite test helpers.


### 6. Scene shaders

Add scene-owned WBOIT shaders.

First native target can be GLSL/SPIR-V:

1. mesh accumulation vertex shader: current mesh/primitive vertex inputs plus scene MVP.
2. mesh accumulation fragment shader: apply color/material alpha and write accumulation outputs.
3. resolve vertex shader: fullscreen triangle.
4. resolve fragment shader: sample accumulation targets and write final color.

WGSL equivalents are desirable if WebGPU parity is part of the same slice, but do not block the
native interactive example unless DRP2 fixture/browser coverage is in scope.


### 7. Tests and validation

Add tests in layers:

1. scene API test: alpha mode stores on visual and marks render-pass assignment dirty.
2. FramePlan test: mixed opaque/transparent panel emits opaque, accumulation, and resolve nodes.
3. capability test: WBOIT request fails with a diagnostic when required capability facts are absent.
4. DRP2 fixture test: multi-attachment accumulation + resolve validates and serializes.
5. vklite/DRP2 runtime smoke: accumulation + resolve produces nonblank output.
6. app/offscreen scene smoke: transparent mesh renders nonblank and does not break depth.
7. manual GLFW example: arcball interaction with transparent mesh remains stable.

Use focused validation first:

```bash
just build
just test drp2
just test scene
just spec-check
```

For runtime graphics changes, also run the narrow vklite/canvas/app path that covers the touched
area. On macOS, use `direnv exec .` for Vulkan-path tests.


## Suggested Implementation Order

1. Done: add public `DvzAlphaMode` and visual storage/accessors, with tests.
2. Done: expand `DvzCapabilitySnapshot` and diagnostics for WBOIT planning.
3. Done for pass roles and first split planning: emit opaque/transparent/resolve FramePlan nodes
   without executing new WBOIT runtime behavior yet. Still pending: explicit accumulation resource
   metadata.
4. Mostly done at the C command/runtime smoke level: DRP2 supports multiple color attachments and
   per-target blend state. Still pending: WBOIT-specific fixture coverage.
5. Done: add DRP2 semantic/schema/fixture coverage for a minimal WBOIT accumulation + resolve
   sequence.
6. Done: teach vklite DRP2 runtime to execute WBOIT accumulation, intermediate texture transitions,
   and resolve.
7. Done: add scene WBOIT accumulation and resolve shaders.
8. Done: lower scene WBOIT FramePlan nodes to explicit DRP2 command streams.
9. Done: add focused scene/vklite WBOIT regression tests.
10. Done: add `examples/c/hello_mesh_wboit_glfw.c`.
11. Done: add live GUI controls for WBOIT cube color, alpha, ambient/diffuse, and light direction.
12. Done: add opaque rear/front reference cards and labeled DRP2 trace metadata so the live example
    can diagnose whether opaque geometry writes depth before WBOIT accumulation. The expected stream
    shape is opaque `rt` pass with depth writes, `_wboit_accum_*` pass with depth test/no depth
    writes, and final `rt` resolve pass with no depth.
13. Done: switch the live example to a light diagnostic background by default, add a GUI toggle for
    the background, and tune the cube/reference defaults so WBOIT face overlap and opaque-card
    occlusion are easier to inspect.
14. Next: add offscreen/capture coverage for WBOIT if needed.


## Intended User API Shape

The user should not manage WBOIT attachments, blend factors, pass ordering, resolve shaders, or
sorting. They should opt a visual into transparent rendering explicitly.

Example shape:

```c
DvzVisual* cube = dvz_mesh(scene, 0);

dvz_visual_set_data(cube, "position", positions, vertex_count);
dvz_visual_set_data(cube, "normal", normals, vertex_count);
dvz_visual_set_data(cube, "color", colors_rgba, vertex_count);
dvz_visual_set_buffer(cube, "index", index_buffer);

dvz_visual_set_alpha_mode(cube, DVZ_ALPHA_WBOIT);

dvz_panel_add_visual(panel, cube, NULL);
dvz_panel_set_arcball(panel, router, 0);
```

Policy:

1. transparent rendering is automatic after explicit opt-in;
2. alpha values remain material/color data;
3. `DVZ_ALPHA_BLENDED` maps to ordinary source-over blending;
4. `DVZ_ALPHA_WBOIT` maps to weighted blended order-independent transparency;
5. no CPU sorting is exposed to the user;
6. no separate transparent mesh visual family is needed;
7. capability failure produces a diagnostic rather than a silent downgrade.


## Non-Goals For The First Slice

1. exact per-pixel linked-list OIT;
2. CPU-sorted alpha fallback;
3. volume transparency integration;
4. WebGPU parity unless explicitly scoped;
5. napari-style blend modes beyond alpha mode;
6. per-item alpha-mode switching.
