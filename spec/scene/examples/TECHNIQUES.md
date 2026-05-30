# Technique Example Notes

> **Status:** informative example guidance
> **Scope:** examples that demonstrate pass-level rendering techniques such as transparency,
> occlusion, EDL, SSAO, MSAA, WBOIT, depth peeling, depth cueing, and materials.

Durable transparency and occlusion semantics live in
[`../semantics/TRANSPARENCY.md`](../semantics/TRANSPARENCY.md), graph-backed technique structure
lives in [`../implementation/GRAPH_TECHNIQUES.md`](../implementation/GRAPH_TECHNIQUES.md), and
occlusion implementation notes live in
[`../implementation/OCCLUSION_EFFECTS.md`](../implementation/OCCLUSION_EFFECTS.md). Keep this file
focused on example pressure and user-facing guidance.


## Transparency And Occlusion Pressure

Mixing volumes, slices, transparent meshes, and object-level visibility is difficult because display
transparency and occlusion semantics are not the same thing:

1. a mesh may be drawn transparently but still be intended to occlude a slice;
2. a hidden or alpha-zero region must not stamp an occlusion buffer;
3. a standalone transparent mesh benefits from order-independent transparency;
4. a transparent mesh embedded in a blended volume/slice composition may be more stable with
   ordinary source-over blending.

The current scene architecture can support this class of examples with explicit render passes and
prepass textures, but one visual alpha mode is not expressive enough to capture every user intent.
Future API work should separate visual compositing policy from occluder policy.


## Allen Brain Slice Pressure Test

The historical live pressure test is:

```text
examples/c/allen_mouse_brain_slice_glfw.c
```

The current example policy is:

1. the volume and slice use `DVZ_ALPHA_BLENDED`;
2. the atlas mesh uses `DVZ_ALPHA_OPAQUE` when all visible regions are effectively alpha 1;
3. the atlas mesh uses `DVZ_ALPHA_BLENDED` when it is transparent and a visible full volume is
   present;
4. the atlas mesh uses `DVZ_ALPHA_WBOIT` when it is transparent and the full volume is hidden;
5. the atlas mesh participates in scene occlusion only when at least one region is effectively
   visible.

WBOIT is useful for standalone transparent atlas meshes, but mixing a WBOIT atlas mesh with the
blended volume/slice path produced unstable slice occlusion and visible discontinuities.
Source-over blending is less general for self-overlapping geometry, but more predictable for the
current volume-plus-slice composition.


## Example Policy

Technique examples should make the intended policy explicit:

1. display alpha mode;
2. depth write/read behavior;
3. scene-occlusion participation;
4. whether alpha affects occlusion;
5. fallback or diagnostic behavior when a backend cannot support the requested technique.

Technique examples should not define new public scene semantics by themselves. Promote reusable
rules to the semantic or implementation specs linked above.


## User-Facing Guidance

Public documentation should not present one universal transparency solution.

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
5. How should documentation phrase the tradeoff between speed, stability, and physical correctness?
