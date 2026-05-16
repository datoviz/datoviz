# Scene Techniques And Materials Plan

> **Execution Status**
> - **Status:** `PICKUP PLAN`
> - **Updated on:** `2026-05-16`
> - **Purpose:** define the next architecture slices for scene effects, materials, and visual pass
>   capabilities without adding more one-off render paths.


## Progress On This Lane

Completed implementation commits:

1. `bc36100e` — extracted scene technique graph planning;
2. `6b85c209` — added internal scene material state;
3. `bbb17c87` — added lit primitive depth cueing;
4. `af19c693` — added scene visual pass capabilities;
5. `310b3cb9` — added the internal G-buffer graph foundation;
6. `f58cec92` — added opt-in G-buffer DRP2 runtime lowering;
7. `0068f1e2` — added internal scene/panel technique activation state;
8. `7e50fe26` — added the first panel-local EDL scene technique;
9. `8981472e` — fixed sampled depth transitions for the EDL/runtime path;
10. `f90774ba` — added the interactive point-cloud EDL GLFW example.

The G-buffer foundation now covers internal eligibility, graph declarations, and opt-in runtime
lowering for primitive/mesh visuals with normals. The current runtime slice emits normal and depth
targets through the existing scene -> FramePlan graph -> DRP2 -> vklite path. G-buffer activation is
now routed through internal scene/panel technique state and remains default-off until a concrete
effect requests it. EDL is the first concrete post-process on this foundation: it is exposed as a
small panel-local public toggle through `dvz_panel_set_edl()`, emits an opaque scene color/depth
intermediate plus a fullscreen resolve pass, and has both offscreen app regression coverage and an
interactive GLFW/GUI example.


## Source Architecture Note

Start with
[../../docs/architecture/scene_techniques_materials.md](/home/cyrille/GIT/Viz/datoviz/docs/architecture/scene_techniques_materials.md).

That note is the architectural target. This file is the practical implementation pickup order.


## Current Repo Reality

Do not assume a material system already exists. Current scene state is split across:

1. `DvzAlphaMode` on visuals;
2. `DvzPrimitiveShadingDesc` and the internal primitive shading uniform;
3. `DvzVolumeState`;
4. scale/colormap bindings;
5. family-specific shader, pipeline, and bind descriptors in `src/scene/visual_pipeline.c`.

The FramePlan graph is active and should be the shared foundation. Current graph-backed paths
include opaque depth, WBOIT, depth peeling, and blended volume. Keep new work on the existing
scene -> FramePlan graph -> DRP2 -> vklite/canvas route.


## Implementation Slices

### 1. Technique Builder Extraction

Scope: behavior-preserving cleanup.

Expected work:

1. add internal files such as `src/scene/technique.c` and `src/scene/_technique.h`, or a narrower
   name if the implementation naturally splits by domain;
2. move opaque-depth, blended, WBOIT, and depth-peeling graph setup out of the middle of
   `scene_emit.c`;
3. keep render-node creation and graph pass/resource names identical;
4. preserve current tests and examples.

Validation:

```text
just build
just test scene
git diff --check
```


### 2. Internal Material State

Scope: add a common retained representation without changing public behavior.

Expected work:

1. add an internal `DvzMaterialState` to `DvzVisual`;
2. initialize default unlit/lit/volume-like material fields in visual construction;
3. map `visual->alpha_mode` into material state while preserving the public getter/setter behavior;
4. route `dvz_visual_set_primitive_shading()` through material-backed fields while preserving the
   current primitive shading uniform payload and tests;
5. expose material state only internally unless a small typed public API is deliberately chosen.

Validation:

```text
just build
just test test_scene_indexed_primitive_shading_updates_runtime
just test test_scene_visual_alpha_mode
git diff --check
```


### 3. Depth Cueing

Scope: first material-driven visual effect.

Expected work:

1. add depth-cue fields to material state;
2. add a typed setter only if needed for examples/tests, for example a small
   `dvz_visual_set_depth_cue()` or panel-level helper;
3. update primitive/mesh shaders first;
4. use view-space or clip-space depth consistently with the existing common MVP uniforms;
5. add an offscreen regression that verifies near/far geometry changes color predictably.

Validation:

```text
just build
just test scene
./build/examples/c/hello_mesh  # or a new bounded depth-cue example once added
git diff --check
```


### 4. Visual Pass Capabilities

Scope: make technique eligibility explicit.

Expected work:

1. add an internal `DvzSceneVisualPassCaps` descriptor;
2. resolve pass capabilities from visual family, material state, attributes, alpha mode, and
   controller mode;
3. replace new technique conditionals with capability checks;
4. add tests for point, pixel, primitive, mesh, path, image, and volume capability expectations.

Validation:

```text
just build
just test scene
git diff --check
```


### 5. G-Buffer Foundation

Scope: graph-backed depth, normal, and object-id resources.

Status: graph foundation landed in `310b3cb9`; opt-in runtime lowering landed in `f58cec92`.

Expected work:

1. keep primitive/mesh visuals with normals as the first eligible family;
2. keep explicit pass roles only where runtime dispatch still requires them;
3. keep the graph as the authoritative resource/pass description;
4. add optional object-id output only when outlines or picking-style effects need it;
5. avoid adding effect-specific descriptor refresh or resize logic.

Validation:

```text
just build
just test scene
just test drp2
git diff --check
```


### 6. Technique Activation And Runtime Policy

Scope: replace the temporary internal G-buffer flag with consistent technique state.

Status: landed in `0068f1e2`.

Expected work:

1. add an internal scene or panel technique-state descriptor for enabled techniques and options;
2. route the G-buffer opt-in through that state while preserving default-off behavior;
3. decide whether effects are scene-wide or panel-local before exposing any public API;
4. keep graph-backed runtime dispatch generic; do not add another per-effect execution path;
5. add tests proving the default scene stream is unchanged and opt-in streams add only requested
   passes.

Validation:

```text
just build
just test scene
just test drp2
git diff --check
```


### 7. Effects On The Shared Foundation

Technique activation is now normalized internally. Implement effects in this order:

1. EDL for point/pixel/particle-heavy panels, using depth only;
2. object-id selected outlines, using a semantic object/group id buffer;
3. SSAO/GTAO for primitive/mesh/sphere-impostor-capable visuals;
4. generic scalar material modulation for curvature/cavity/accessibility/uncertainty channels.

Current EDL status: the first native GLSL/vklite slice is implemented for opaque point/pixel and
opaque depth-writing visuals in the simple opaque graph branch. It intentionally remains
default-off and does not yet compose with WBOIT, depth peeling, or blended volume branches. The next
useful EDL follow-up is to route point/pixel depth-writing through the same visual capability
metadata used by mesh/primitive effects, then decide whether EDL should become a generic graph
post-process that can run after selected transparent techniques.

Use the existing SSAO implementation plan for the SSAO-specific graph and shader details:
[SCENE_SSAO_IMPLEMENTATION_PLAN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/SCENE_SSAO_IMPLEMENTATION_PLAN.md).


## Guardrails

1. Do not add a public framegraph API for this lane.
2. Do not add a parallel renderer or scene-private Vulkan execution path.
3. Do not hardcode molecular semantics into core materials.
4. Keep public APIs typed and narrow.
5. Prefer internal state and compatibility mapping before broad public `DvzMaterial` exposure.
6. Keep examples and tests in lockstep with each new technique.
