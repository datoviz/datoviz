# Transparency And Depth Ordering

This document defines how the scene layer handles transparent visuals, depth ordering,
and order-independent transparency.


## Alpha Modes

Every visual declares an `alpha_mode` that tells the scene how to handle its fragments:

| Mode | Description |
|---|---|
| `DVZ_ALPHA_OPAQUE` | all fragments fully opaque; depth test enabled, depth write enabled |
| `DVZ_ALPHA_BLENDED` | per-fragment alpha blending; weighted blended OIT (default transparent path) |
| `DVZ_ALPHA_BLENDED_EXACT` | per-fragment alpha blending; per-pixel linked list OIT (exact, capability-gated) |
| `DVZ_ALPHA_MASK` | binary alpha cutout (alpha < threshold → discard); depth write enabled |

`DVZ_ALPHA_OPAQUE` is the default.
`DVZ_ALPHA_MASK` is useful for foliage, text impostor quads, and marker sprites where
hard edges are acceptable and depth write must be preserved.


## Render Pass Structure

The scene splits rendering into two ordered render passes per panel:

1. **Opaque pass** — all `DVZ_ALPHA_OPAQUE` and `DVZ_ALPHA_MASK` visuals,
   depth test and depth write enabled.
2. **Transparent pass** — all `DVZ_ALPHA_BLENDED` and `DVZ_ALPHA_BLENDED_EXACT` visuals,
   depth test enabled, depth write disabled.

The transparent pass always executes after the opaque pass.
Opaque visuals are never rendered after transparent visuals in the same panel.

This split is inserted automatically by the scene during frame planning.
The user does not need to specify pass ordering explicitly.


## Weighted Blended OIT (Default)

Weighted blended OIT (McGuire & Bavoil 2013) is the default path for
`DVZ_ALPHA_BLENDED` visuals.

**How it works:**

1. The transparent pass writes to two accumulation targets:
   - `accum`: `rgba_f16` — weighted color accumulation
   - `reveal`: `r_f16` — transmittance accumulation
2. A resolve pass composites the accumulation targets onto the opaque background.

**Properties:**

- No geometry sorting required
- Single extra render pass and two small render targets
- Approximate — ordering artifacts can appear for large overlapping opaque-alpha regions
- Scales to millions of transparent fragments without CPU sort overhead
- Works on all hardware that supports floating-point render targets

The resolve pass is a fullscreen quad inserted automatically into the `FramePlan`
after the transparent pass.


## Per-Pixel Linked List OIT (Exact)

`DVZ_ALPHA_BLENDED_EXACT` uses per-pixel linked list OIT for exact transparency.

**How it works:**

1. The transparent pass writes fragment records into an atomic append buffer
   (one linked list per screen pixel).
2. A sort-and-resolve pass reads the per-pixel lists, sorts by depth, and composites.

**Properties:**

- Exact result for any number of overlapping layers
- Requires GPU atomic operations and large auxiliary buffers (proportional to
  screen resolution × average overdepth)
- Capability-gated: requires `DVZ_CAP_ATOMIC_FRAGMENT_STORE`
- Falls back to `DVZ_ALPHA_BLENDED` if the capability is absent, with a diagnostic

`DVZ_ALPHA_BLENDED_EXACT` is appropriate for dense molecular visualization, medical
imaging overlays, or any scene where OIT approximation errors are unacceptable.


## CPU Depth Sort (Fallback)

For `DVZ_ALPHA_BLENDED` visuals on hardware that does not support floating-point
render targets (rare), the scene falls back to CPU depth sort:

1. item world positions are read from the CPU-side copy of the position buffer,
2. they are sorted by depth (distance from camera) each frame,
3. the sorted index array is uploaded as a draw-order index buffer.

This fallback is O(N log N) per frame and breaks for intersecting geometry.
It is capability-controlled and emits a diagnostic when active.

For static scenes or scenes where items do not intersect, it produces correct results.


## Volume Visuals

Volume visuals (`DVZ_VISUAL_VOLUME`) use fragment shader ray casting and handle their
own internal compositing.
They are exempt from the OIT passes.
Volumes are rendered in their own pass, typically after the opaque pass and before the
transparent compositing step.

Multiple overlapping volume visuals in the same panel blend correctly. Each volume's
transparent fragments enter the standard WB-OIT accumulation pass. No special ordering is
required — this is an inherent property of weighted blended OIT.


## Interaction With Selection And Highlight

The selection mask buffer and highlight descriptor apply to transparent visuals in the
same way as opaque visuals.
The highlight alpha multiplier modifies the per-fragment alpha before OIT accumulation.

`selected_z_layer` is not meaningful for `DVZ_ALPHA_BLENDED` or
`DVZ_ALPHA_BLENDED_EXACT` visuals because depth write is disabled in the transparent
pass.
The scene emits a diagnostic if `selected_z_layer ≠ 0` is declared on a transparent
visual.


## FramePlan Structure

For a panel containing both opaque and transparent visuals:

```text
FramePlan (panel):
  RenderNode  — opaque pass  (DVZ_ALPHA_OPAQUE, DVZ_ALPHA_MASK visuals)
  RenderNode  — transparent accumulation pass  (DVZ_ALPHA_BLENDED visuals)
  RenderNode  — OIT resolve pass  (fullscreen composite)
  RenderNode  — volume pass  (DVZ_VISUAL_VOLUME visuals)
```

Panels with no transparent visuals have no accumulation or resolve nodes.
The scene omits those nodes during frame planning — there is no per-frame overhead
for purely opaque panels.


## Declaring Alpha Mode On A Visual

Alpha mode is a visual-level property, not a per-item property:

```text
dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_BLENDED)
```

It can be changed at any time.
Changing `alpha_mode` marks the visual's render-pass assignment dirty and triggers
a `FramePlan` rebuild for the affected panel.

Per-item alpha is expressed through the item's color alpha channel or an opacity scale.
`alpha_mode` controls the rendering path, not the per-item opacity value.


## Relationship To Other Documents

| Document | Relationship |
|---|---|
| `FRAME_PLAN_IR.md` | render pass split and resolve node in FramePlan |
| `CAPABILITY_ADAPTATION.md` | exact OIT fallback to weighted OIT; weighted OIT fallback to CPU sort |
| `SELECTION.md` | highlight alpha multiplier applied before OIT accumulation |
| `LIGHTING.md` | transparent visuals with PBR shading use the same OIT paths |
| `VISUAL_CONTRACT.md` | custom visuals must declare their alpha mode |
