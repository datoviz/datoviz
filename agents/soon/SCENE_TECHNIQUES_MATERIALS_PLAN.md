# Scene Techniques And Materials Plan

> **Execution Status**
> - **Status:** `IMPLEMENTED THROUGH SPHERE SSAO`
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
10. `f90774ba` — added the interactive point-cloud EDL GLFW example;
11. `52870408` — enabled depth testing for point visuals;
12. `bbc729f2` — added depth cueing for point visuals;
13. `4748026a` — introduced shared scene material parameters;
14. `23d1baf1` — refactored scene shader variant selection;
15. `f42e03ae` — factored visual material update helpers;
16. `f47c8e90` — shared scene material shader helpers;
17. `5f390337` — extended depth cue metrics and falloff;
18. `b90a6383` — clarified scene visual pass capabilities;
19. `1ca7979b` — extracted depth post-process graph planning;
20. `dcfea8c8` — added the SSAO graph planning foundation;
21. `ac8769e9` — added SSAO runtime lowering;
22. `2f132123` — added SSAO runtime smoke coverage;
23. `e28645d7` — added offscreen SSAO image-difference coverage;
24. `d2b9a90b` — added the standalone sphere impostor visual;
25. `f0765368` — used sphere impostors in the SSAO example;
26. `fb98fc55` — improved sphere material highlights and antialiasing;
27. `0548bca8` — stabilized sphere edges and SSAO depth;
28. `64cd5cc5` — added raycast sphere impostor mode;
29. `3e73bb72` — added the sphere raycast mode toggle to the SSAO example;
30. `8289e423` — improved scene SSAO sampling and composite;
31. `db55e05e` — added the SSAO bilateral blur pass;
32. `9687fa0e` — tuned sphere SSAO example defaults;
33. `8011973a` — added DRP2 multisample protocol fields;
34. `8cc9d4fa` — added scene panel MSAA lowering;
35. `7782d9fc` — used the named MSAA resolve mode in scene graph lowering;
36. `d1a472ee` — covered point visuals in the EDL depth-producer regression.

The G-buffer foundation now covers internal eligibility, graph declarations, and opt-in runtime
lowering for primitive/mesh visuals with normals. The current runtime slice emits normal and depth
targets through the existing scene -> FramePlan graph -> DRP2 -> vklite path. G-buffer activation is
now routed through internal scene/panel technique state and remains default-off until a concrete
effect requests it. EDL is the first concrete post-process on this foundation: it is exposed as a
small panel-local public toggle through `dvz_panel_set_edl()`, emits an opaque scene color/depth
intermediate plus a fullscreen resolve pass, and has both offscreen app regression coverage and an
interactive GLFW/GUI example. SSAO is now also exposed as a panel-local technique, uses the same
G-buffer foundation, supports optional bilateral blur, and has graph, DRP2 runtime, offscreen image
difference, and sphere-impostor runtime coverage. Sphere is a standalone retained visual family with
analytic impostor rendering, material lighting, antialiased silhouettes, raycast mode, G-buffer
output, and a GLFW/GUI SSAO example. MSAA is implemented as a panel-local graph-backed technique
with DRP2 multisample lowering and explicit resolve metadata.


## Source Architecture Note

Start with
[scene_techniques_materials.md](../../docs/architecture/scene_techniques_materials.md).

That note is the architectural target. This file is the practical implementation pickup order.


## Current Repo Reality

Do not assume a public `DvzMaterial` object exists. The current retained material policy is an
internal compatibility layer split across:

1. `DvzAlphaMode` on visuals;
2. internal `DvzMaterialState` / `DvzSceneMaterialParams` on visuals;
3. `DvzPrimitiveShadingDesc` as the typed compatibility setter for lit material fields;
4. `DvzVolumeState`;
5. scale/colormap bindings;
6. family-specific shader, pipeline, and bind descriptors in `src/scene/visual_pipeline.c`.

The FramePlan graph is active and should be the shared foundation. Current graph-backed paths
include opaque depth, WBOIT, depth peeling, blended volume, G-buffer, EDL, SSAO, SSAO blur, and
MSAA resolve. Keep new work on the existing scene -> FramePlan graph -> DRP2 -> vklite/canvas route.


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

1. EDL for point/pixel/particle-heavy panels, using depth only — implemented for opaque
   depth-producing visuals and guarded by visual capability metadata;
2. SSAO/GTAO for primitive/mesh/sphere-impostor-capable visuals — implemented as graph-backed SSAO
   with optional bilateral blur;
3. sphere impostors as the dense-particle/atom visual family — implemented with color, G-buffer, and
   SSAO coverage;
4. object-id selected outlines, using a semantic object/group id buffer — still deferred until the
   selection/semantic-id contract is made explicit;
5. generic scalar material modulation for curvature/cavity/accessibility/uncertainty channels —
   still deferred until scalar slots are represented in retained visual/material state.

Current EDL status: the native GLSL/vklite slice is implemented for opaque point/pixel and opaque
depth-writing visuals in the simple opaque graph branch, and point/pixel eligibility is now routed
through the same visual capability metadata as primitive, mesh, path, and sphere effects. It
intentionally remains default-off and does not yet compose with WBOIT, depth peeling, blended volume,
or SSAO branches. The next EDL architecture decision is whether it should become a generic graph
post-process that can run after selected transparent techniques.

Use the existing SSAO implementation plan for the SSAO-specific graph and shader details:
[SCENE_SSAO_IMPLEMENTATION_PLAN.md](SCENE_SSAO_IMPLEMENTATION_PLAN.md).

Sphere impostors have been implemented as a standalone visual family rather than a marker variant.
The v0.4 pickup plan and remaining texture/equirectangular follow-ups are recorded in
[SPHERE.md](../../spec/scene/visuals/SPHERE.md).


## Guardrails

1. Do not add a public framegraph API for this lane.
2. Do not add a parallel renderer or scene-private Vulkan execution path.
3. Do not hardcode molecular semantics into core materials.
4. Keep public APIs typed and narrow.
5. Prefer internal state and compatibility mapping before broad public `DvzMaterial` exposure.
6. Keep examples and tests in lockstep with each new technique.
