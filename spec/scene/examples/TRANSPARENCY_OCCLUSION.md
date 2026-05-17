# Transparency And Occlusion Notes

> **Agent Pickup**
> - **Category:** cross-cutting scene example note
> - **Implementation target:** document the current policy and open design space for examples that
>   mix volumes, slices, and transparent geometry.
> - **Scope:** informative. This is not yet a normative API contract.
> - **Validation:** use the Allen mouse brain slice example as the current live pressure test.


## Summary

Mixing volumes, slices, transparent meshes, and object-level visibility is difficult to make both
correct and fast. The hard part is that display transparency and occlusion semantics are not the
same thing:

1. a mesh may be drawn transparently but still be intended to occlude a slice,
2. a hidden or alpha-zero region must not stamp an occlusion buffer,
3. a standalone transparent mesh benefits from order-independent transparency,
4. a transparent mesh embedded in a blended volume/slice composition may be more stable with
   ordinary source-over blending.

The current scene architecture can support this class of examples with explicit render passes and
prepass textures, but a single visual alpha mode is not expressive enough to capture every user
intent. Future API work should separate visual compositing policy from occluder policy.


## Current Allen Example Outcome

The live pressure test is:

```text
examples/c/allen_mouse_brain_slice_glfw.c
```

The current policy is:

1. the volume and slice use `DVZ_ALPHA_BLENDED`,
2. the atlas mesh uses `DVZ_ALPHA_OPAQUE` when all visible regions are effectively alpha 1,
3. the atlas mesh uses `DVZ_ALPHA_BLENDED` when it is transparent and a visible full volume is
   present,
4. the atlas mesh uses `DVZ_ALPHA_WBOIT` when it is transparent and the full volume is hidden,
5. the atlas mesh participates in scene occlusion only when at least one region is effectively
   visible.

The experiments leading to this policy found that WBOIT is useful for standalone transparent atlas
meshes, but mixing a WBOIT atlas mesh with the blended volume/slice path produced unstable slice
occlusion and visible discontinuities. Source-over blending is less general for self-overlapping
geometry, but it is more predictable for the current volume-plus-slice composition.


## Scene Occlusion Prepass Policy

The scene-occlusion prepass writes a single sampled depth texture used by scene-occluded visuals.
For primitive mesh occluders, the prepass should:

1. discard alpha-zero fragments so hidden regions do not occlude,
2. use a depth attachment and write depth,
3. compare with `LESS_OR_EQUAL` so multiple occluders resolve by nearest depth instead of draw
   order,
4. avoid material, image, and scene-occlusion bindings that the depth-only shader does not use.

This makes the prepass an explicit occlusion proxy. It is related to but separate from the final
color pass.


## Why This Is Hard

The common raster techniques solve different subsets of the problem:

1. **Opaque depth:** fast and stable, but cannot show transparent interiors.
2. **Source-over blending:** simple and useful when draw order is controlled, but wrong for many
   self-overlapping transparent surfaces.
3. **WBOIT:** fast approximate order-independent transparency, but it is a color accumulation
   technique and does not by itself define solid occlusion semantics.
4. **Depth peeling:** better for layered transparent geometry, but more expensive and still needs
   a policy for how volume samples and surfaces combine.
5. **Volume ray marching:** natural for volume/slice effects, but meshes are still separate unless
   surface intersections are integrated into the ray.
6. **Ray tracing or ray-guided hybrid rendering:** can express the most physically coherent
   visibility model, but it is a larger backend and performance commitment.

Scientific visualization systems normally expose more than one technique because user intent
varies. An anatomical atlas shell, a glass material, a confidence hull, an annotation surface, and a
semi-transparent segmentation label may all need different occlusion semantics even when they look
similar on screen.


## Is The Current Architecture Reasonable?

The current Datoviz direction is reasonable for a raster scene engine:

1. retain visuals and emit a frame plan,
2. use explicit graph passes for depth, occlusion, transparent accumulation, and final color,
3. keep fast paths for common interactive examples,
4. add focused policies in examples where user intent is known.

The Allen example also shows a limit of the current abstraction. `DvzAlphaMode` currently couples
several decisions:

1. which color pass draws the visual,
2. whether the visual writes or reads depth in that pass,
3. how the visual interacts with other transparent visuals,
4. indirectly, how users expect it to behave as an occluder.

Those are not always the same decision.


## Likely Improvements

Short-term improvements that fit the current architecture:

1. make occluder policy explicit, for example "visual alpha affects occlusion", "occlude when
   alpha greater than threshold", or "use a dedicated occluder proxy",
2. allow a visual to choose a display alpha mode independently from its scene-occlusion mode,
3. use one visual per atlas region when the expected visible region count is small,
4. keep the single-batched-visual path for many labels that are mostly global and not individually
   manipulated,
5. add debug overlays for scene-occlusion depth, volume-occlusion depth, and final depth.

Medium-term improvements:

1. add a depth-peeling example that mixes transparent meshes and a volume/slice,
2. make pass graph diagnostics easier to inspect from app logs,
3. document recommended policies per use case in the public scene documentation,
4. provide a small set of named transparency presets instead of requiring users to understand every
   pass detail.

Long-term options:

1. hybrid volume/surface compositing where the volume ray marcher samples or tests surface depth
   layers,
2. ray tracing or ray-query backends for scenes that require physically coherent transparent
   visibility,
3. multi-layer depth or A-buffer style techniques for higher-quality OIT when performance permits.


## Recommended User-Facing Guidance

The eventual user documentation should not present one universal transparency solution. It should
state that transparency is use-case dependent and offer a decision table:

| Use case | Recommended policy |
| --- | --- |
| Solid mesh with volume/slice occlusion | Opaque mesh plus scene-occlusion prepass |
| Transparent mesh with no volume | WBOIT |
| Transparent atlas shell with visible volume/slice | Source-over mesh plus scene-occlusion proxy |
| Few individually toggled atlas regions | One visual per region |
| Many mostly-global labels | Batched visual with alpha-aware occlusion shader |
| High-quality layered transparent surfaces | Depth peeling or future layered OIT |
| Physically coherent volume/surface transparency | Future hybrid ray marching or ray tracing |


## Open Questions

1. Should scene occlusion be controlled by a separate public API from alpha mode?
2. Should atlas/label visuals have first-class per-region visual objects rather than alpha-zero
   hidden regions in one visual?
3. Should the scene graph expose diagnostic views for prepass textures?
4. Which transparency presets should be public, and which should remain internal policy choices?
5. How should the documentation phrase the tradeoff between speed, stability, and physical
   correctness?
