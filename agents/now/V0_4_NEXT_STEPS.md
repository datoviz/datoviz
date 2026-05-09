# Datoviz v0.4 Next Steps

> **Execution Status**
> - **Status:** `ACTIVE DEVELOPMENT GUIDE`
> - **Updated on:** `2026-05-09`
> - **Purpose:** orient near-term and mid-term v0.4 work after the first scene -> DRP2 -> vklite/canvas slice.


## Current Position

Datoviz v0.4 now has a real higher-level vertical slice, not just low-level infrastructure.

Focused validation recorded on `2026-05-06`:

1. `just spec-check`: `119/119` DRP2 fixtures passed; `52` fixture-runner tests passed.
2. `just test drp2`: `73/73` tests passed.
3. `just test scene`: `52/52` tests passed.
4. `git diff --check`: passed for the last documentation-only refresh.

Recent follow-up validation on `2026-05-06`:

1. `just test scene`: passed after clear-plan and retained-render updates.
2. `just test vklite_swapchain`: passed with automated present windows hidden by default.
3. `DVZ_TEST_VISIBLE=1 just test vklite_swapchain`: terminated after hanging in visible
   compositor-dependent present execution; keep visible mode manual/debug-only.

The low-level stack is the current foundation:

1. `vk` owns low-level Vulkan instance/device/queue/memory primitives.
2. `vklite` owns higher-level Vulkan wrappers for buffers, images, commands, graphics, descriptors,
   compute, rendering, swapchain, and sync.
3. `canvas` owns frame acquisition, borrowed frame command buffers, swapchain/offscreen targets, and
   stream submission.
4. `stream` and sinks route frames to swapchain, offscreen, live image, and video consumers.

The active higher layer now exists:

1. `drp2` owns backend-agnostic command streams, JSON/debug serialization, validation, and the
   native vklite runtime.
2. `scene` owns early scene graph objects, capability snapshots, diagnostic reports, frame plans,
   DRP2 emission, and a minimal app/offscreen path.
3. Built-in visual families currently implemented are `point`, `primitive`, and `image`.
4. Current examples cover `hello_point`, `hello_scatter`, `hello_triangle`, `hello_texture`,
   `hello_point_glfw`, `raw_triangle`, and `raw_triangle_drp2`.
5. Panel controllers are live: panzoom and arcball are connected through the input router and feed
   per-panel transform UBOs.
6. Per-panel runtime viewport/scissor is working through the emitted DRP2 path.
7. The app/offscreen path preserves prior panel contents correctly across later `LOAD` render passes.


## What Is Complete

The following earlier priorities are no longer the active plan:

1. Finish the first point-based scene slice.
2. Add a minimal `primitive` family.
3. Add a minimal `image` / texture family.
4. Harden the first scene/DRP2 runtime failure and readback paths.
5. Keep simple examples in lockstep with the first API surface.
6. Port panzoom and arcball and wire controller-driven panel transforms.

Those implementation records now live primarily in:

1. [done/SCENE_DRP2_IMPLEMENTATION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/SCENE_DRP2_IMPLEMENTATION.md)
2. [done/DRP2_SCENE_SAFETY.md](/home/cyrille/GIT/Viz/datoviz/agents/done/DRP2_SCENE_SAFETY.md)
3. [done/CONTROLLER_TRANSFORM_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/done/CONTROLLER_TRANSFORM_DESIGN.md)


## Roadmap Principles

The next plan should stay linear and pressure the architecture in the right order.

1. Add the minimum new scene surface that proves the runtime boundary, not a broad visual zoo.
2. Force a real 3D slice early so depth testing, depth attachments, viewport UBO usage, and arcball
   stop being only partially exercised features.
3. Run a browser/WebGPU feasibility pass before too many native-only assumptions harden.
4. Delay heavier visual families until the first 3D slice and browser pressure have clarified the
   right DRP2/runtime seams.
5. Treat transparency and picking as architectural pressure tests, not only as feature requests.
6. Use real examples to drive the sequence, especially one 3D scientific example instead of only
   abstract smoke tests.


## Recommended Phase Order

### Phase 1. Native 3D baseline

This is the immediate priority.

Goal:

Prove one real indexed 3D visual on the existing native stack and make depth/state/view transforms
first-class in the active implementation, not just in specs and controller plumbing.

Deliverables:

1. Minimal `mesh` visual family for indexed triangle geometry.
2. Per-panel depth attachment ownership and runtime wiring.
3. Viewport UBO completed and used intentionally by built-in visuals that need panel-pixel context.
4. Arcball-driven 3D example and regression coverage.
5. Focused DRP2/runtime tests that exercise depth compare/write state and viewport/scissor updates.
6. A written mesh shading/material contract that keeps the first implementation narrow while leaving
   room for later contour/isoline and PBR work.

Scope notes:

1. Keep the first `mesh` family narrow: indexed triangle list, position/color/normal, optional
   simple lighting, optional texture later.
2. Do not start with mesh isolines, edge overlays, full material systems, or grouped region
   semantics.
3. Depth attachment ownership must remain with canvas/runtime boundaries, not scene.
4. Scene should keep emitting frame plans and DRP2; it should not begin owning pass lifecycle
   details directly.
5. Use [MESH_SHADING_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/MESH_SHADING_DESIGN.md)
   as the Phase 1 shading/material contract.

Exit criteria:

1. One offscreen `hello_mesh` example renders a lit rotating or arcball-controlled mesh.
2. One GLFW live 3D example proves arcball, depth testing, and retained rendering together.
3. `just test scene` and `just test drp2` gain explicit 3D/depth coverage.

### Phase 2. Browser/WebGPU feasibility pass

This should happen earlier than a long tail of extra visual families.

Goal:

Test whether the current DRP2 command surface, object model, and scene emission choices remain
clean when replayed against a minimal browser-side WebGPU runtime.

Why here:

1. After Phase 1 there is enough surface area to pressure 2D, textured, and 3D basics.
2. Before axes, transparency, and many extra families land, it is still cheap to correct
   Vulkan-biased assumptions.
3. Browser feasibility does not need full feature parity; it needs a real proof that the contract
   is still portable.

Deliverables:

1. Minimal browser runtime that can replay a narrow DRP2 subset:
   `buffer`, `texture`, `sampler`, `bind group`, `render pipeline`, `viewport/scissor`, `draw`,
   `submit`, and readback where feasible.
2. A fixture or example parity lane for:
   `point`, `primitive`, `image`, and one minimal `mesh`/depth scene.
3. A short gap report listing DRP2/runtime assumptions that remain Vulkan-shaped.

Explicit non-goals:

1. Do not block native scene work on full browser parity.
2. Do not attempt transparency, advanced text, or volume support in the first browser pass.
3. Do not widen DRP2 aggressively only to mirror every native detail.

Exit criteria:

1. Browser replay of at least one 2D and one 3D fixture works.
2. Any portability gaps are written down concretely and fed back into DRP2/spec/runtime work.

### Phase 3. Transparency architecture pass

Goal:

Pressure the render graph and frame-plan design with a real multi-pass feature that native users
care about and that will expose whether DRP2/scene boundaries are sufficient.

Recommended first target:

1. Weighted blended OIT or another bounded-pass transparency technique that does not require
   per-pixel linked lists as the first implementation.

Why before a broad visual expansion:

1. Transparency pressures pass ordering, extra render targets, blending state, and composition.
2. It will reveal whether the frame-plan / converter / runtime boundaries are ready for nontrivial
   multi-pass scene execution.
3. It is directly relevant to the intended brain-shell example and other scientific 3D scenes.

Deliverables:

1. One explicit scene transparency mode wired through scene -> frame plan -> DRP2 -> runtime.
2. Capability-gated fallback behavior when the technique is unavailable.
3. One real-world example:
   semi-transparent outer brain shell + opaque inner region meshes.

Exit criteria:

1. The example renders correctly offscreen and in a live window.
2. The plan/runtime path survives the extra passes without scene leaking low-level ownership.

### Phase 4. Axes, lines, and scene annotation baseline

Goal:

Move beyond free-floating visuals and prove that Datoviz can build an actual scientific figure
surface.

Recommended ordering inside this phase:

1. `line` or `segment` baseline suitable for traces, crosshairs, and grid primitives.
2. Minimal axes infrastructure:
   panel-to-domain mapping, ticks/grid/axis lines, controller-aware updates.
3. Minimal text path only to the extent needed for axis labels/tick labels and simple annotations.

Scope notes:

1. Axes are not a decorative add-on; they are a figure-system milestone.
2. Keep the first axes slice small and controller-aware.
3. Avoid overcommitting to a full retained text engine before the axis/label requirements are clear.

Deliverables:

1. One 2D example with linked panels, panzoom, and controller-updated axes.
2. One 3D example with basic orientation aids or scene annotations where appropriate.
3. Focused tests around domain changes, tick regeneration, and viewport-relative overlays.

### Phase 5. Picking baseline

Goal:

Make interactive scene query paths real and stable across 2D and 3D visuals.

Recommended picking order:

1. Point/image picking stabilization on the active slice.
2. Mesh face picking on the new 3D path.
3. Example-driven picking flows:
   hover probe, click selection, and linked highlight between panels.

Deliverables:

1. Stable pick-id path with scene-side resolution tables.
2. Blocking and/or request-based helpers that fit the active scene ownership model.
3. One 2D picking example and one 3D picking example.

Exit criteria:

1. Picking survives batching and retained rendering.
2. Picking readback does not force scene to own render-target internals.

### Phase 6. Broader visual family expansion

Only after the phases above are grounded.

Likely next families:

1. `text`
2. `marker`
3. richer `line` / `path`
4. `sphere` impostor or mesh-backed variant
5. `volume` slice, then broader volume work

Rule:

Every new family should land with a pressure example, not only an isolated smoke test.


## Immediate Task List

If work resumes right now, this is the preferred order:

1. Use [MESH_SHADING_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/MESH_SHADING_DESIGN.md)
   to freeze the first mesh geometry/material/light scope.
2. Implement per-panel depth attachment support and add tests before broadening the mesh API.
3. Start the first narrow `mesh` implementation and keep the DRP2/spec lane aligned with it.
4. Land the first `mesh` family plus one offscreen and one live example.
5. Finish viewport-UBO usage for the built-in visuals that need pixel-aware sizing or overlays.
6. Run a minimal browser/WebGPU experiment against the resulting 2D+3D slice.
7. Use the findings from that experiment to decide whether DRP2 needs small contract corrections
   before transparency and axes work begin.


## Example-Driven Milestones

The roadmap should be validated against concrete examples, not only per-feature tests.

Near-term examples:

1. `hello_mesh` offscreen
2. `hello_mesh_glfw` with arcball
3. `linked_panels_axes_panzoom` implementation pass
4. `mesh_picking` or equivalent minimal face-picking demo

Flagship pressure example:

1. semi-transparent 3D brain shell
2. opaque region meshes inside
3. arcball navigation
4. picking-driven region selection
5. one linked 2D subplot or probe panel

This example should be treated as an architectural target, not merely a showcase. It combines the
exact pressures that matter next: 3D depth, transparency, picking, multi-panel coordination, and
scene-level identity management.


## DRP2 / Scene Implications To Track

Upcoming feature work should continue to pressure the executable contract deliberately.

1. `mesh` and depth work will pressure pipeline state, depth attachments, and capability reporting.
2. Viewport UBO and axes/text overlays will pressure panel-relative data flow and overlay semantics.
3. Browser/WebGPU replay will pressure shader assumptions, object lifetime semantics, and any
   native-only shortcuts.
4. Transparency will pressure multi-pass composition and feature fallback rules.
5. Picking will pressure readback contracts, logical identity encoding, and frame-plan invalidation.


## Validation Defaults

For docs-only or plan-only changes:

```bash
git diff --check
```

For scene/DRP2 CPU-surface changes:

```bash
just build
just test drp2
just test scene
just spec-check
git diff --check
```

For changes touching borrowed canvas frames, command buffers, render targets, synchronization,
depth attachments, or presentation:

```bash
just build
just test drp2
just test scene
just test canvas
just test vk
just spec-check
git diff --check
```


## Do Not Reopen By Default

1. Do not reintroduce a parallel presentation path for scene.
2. Do not let scene own swapchain, command-buffer begin/end, sink submission, or Vulkan
   synchronization.
3. Do not broaden the first `mesh` implementation into a full renderer subsystem.
4. Do not postpone browser/WebGPU feasibility until after many more native-only visual families
   have landed.
5. Do not jump directly to order-independent transparency before the minimal 3D/depth slice is real.
6. Do not activate dormant modules such as `color`, `wasm`, or broad renderer/client layers unless
   the task explicitly asks for them.
7. Do not carry v0.3 compatibility constraints into this branch.
