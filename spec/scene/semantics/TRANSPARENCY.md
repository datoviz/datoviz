# Transparency And Depth Ordering

This document defines how the scene layer handles transparent visuals, depth ordering,
and order-independent transparency.


## Alpha Modes

Every visual declares an `alpha_mode` that tells the scene how to handle its fragments:

| Mode | Description |
|---|---|
| `DVZ_ALPHA_OPAQUE` | all fragments fully opaque; depth test enabled, depth write enabled |
| `DVZ_ALPHA_BLENDED` | ordinary transparent pass using the visual's blend mode; depth test enabled, depth write disabled |
| `DVZ_ALPHA_WBOIT` | weighted blended order-independent transparency; depth test enabled, depth write disabled |
| `DVZ_ALPHA_DEPTH_PEEL` | depth-peeling transparency; depth test enabled, depth write controlled by peeling passes |
| `DVZ_ALPHA_MASK` | binary alpha cutout (alpha < threshold → discard); depth write enabled |

`DVZ_ALPHA_OPAQUE` is the default.
`DVZ_ALPHA_WBOIT` is the active order-independent transparency path.
`DVZ_ALPHA_MASK` is useful for foliage, text impostor quads, and marker sprites where
hard edges are acceptable and depth write must be preserved.


## Blend Modes

Blend mode is independent from alpha mode. It controls how a visual in the ordinary transparent
pass combines its fragment output with the current color target:

| Mode | Linear RGB equation | Purpose |
|---|---|---|
| `DVZ_BLEND_SOURCE_OVER` | `src.rgb * src.a + dst.rgb * (1 - src.a)` | ordinary transparency |
| `DVZ_BLEND_ADDITIVE` | `src.rgb * src.a + dst.rgb` | luminous particles, density, glow, and emission |

Both modes preserve source-over alpha coverage:

```text
out.a = src.a + dst.a * (1 - src.a)
```

`DVZ_BLEND_SOURCE_OVER` is the default. `DVZ_BLEND_ADDITIVE` requires
`DVZ_ALPHA_BLENDED`; combining it with WBOIT or depth peeling is invalid because those techniques
own their accumulation equations. Opaque and masked visuals ignore blend mode while retaining it as
visual state, so callers may configure blend mode before switching alpha mode.


## Render Pass Structure

The scene splits rendering into ordered passes per panel:

1. **Opaque pass** — all `DVZ_ALPHA_OPAQUE` and `DVZ_ALPHA_MASK` visuals,
   depth test and depth write enabled.
2. **Ordinary transparent pass** — `DVZ_ALPHA_BLENDED` visuals, depth test enabled,
   depth write disabled.
3. **WBOIT accumulation/resolve passes** — `DVZ_ALPHA_WBOIT` visuals.
4. **Depth-peeling passes** — `DVZ_ALPHA_DEPTH_PEEL` visuals when requested.

Transparent passes always execute after the opaque pass.
Opaque visuals are never rendered after transparent visuals in the same panel.

This split is inserted automatically by the scene during frame planning.
The user does not need to specify pass ordering explicitly.

## Linear Color Contract

Standard alpha blending, order-independent transparency, WBOIT accumulation, revealage, and resolve
passes operate in linear color space. Accumulation targets such as `rgba_f16` store linear color.
WBOIT must not accumulate sRGB-encoded colors.

The final transparent/composited output remains linear until final display or standard image-export
encoding. Alpha, revealage, and transmittance are scalar linear quantities and are not
gamma-corrected.

See [COLOR_MANAGEMENT.md](COLOR_MANAGEMENT.md).


## Ordinary Transparent Blending

`DVZ_ALPHA_BLENDED` uses `DVZ_BLEND_SOURCE_OVER` by default. A visual may select
`DVZ_BLEND_ADDITIVE` without changing its render-pass assignment.

Source-over is useful for simple overlays and already-ordered transparent geometry. It is not
order-independent; intersecting or unsorted transparent geometry can show ordering artifacts.
Additive blending is order-independent for finite nonnegative RGB contributions, although the
target format may still saturate.


## Weighted Blended OIT (Active OIT Path)

Weighted blended OIT (McGuire & Bavoil 2013) is the active path for
`DVZ_ALPHA_WBOIT` visuals.

The first WBOIT slice is implemented in the active scene -> DRP2 -> runtime stack for retained
visuals that opt into `DVZ_ALPHA_WBOIT`.

It is selected from lower-level runtime capabilities rather than from a standalone WBOIT flag. The
required ingredients are compatible floating-point render targets, enough color attachments for the
accumulation and reveal targets, color blending support, and the ability to run the accumulation and
resolve passes.

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
- Works on hardware with the required floating-point render-target and blending support

The resolve pass is inserted automatically into the `FramePlan` after the WBOIT accumulation pass.


## Depth Peeling

`DVZ_ALPHA_DEPTH_PEEL` uses depth peeling for higher-quality order-independent transparency.

**How it works:**

1. The transparent path peels successive depth layers.
2. The peeled layers are composited in order.

**Properties:**

- More accurate than weighted blended OIT for difficult overlapping transparent geometry
- More expensive than WBOIT because it needs multiple passes
- Capability- and pass-budget-gated

`DVZ_ALPHA_DEPTH_PEEL` is appropriate when WBOIT approximation errors are unacceptable and the
runtime can afford the extra passes.


## Deferred Exact OIT

Per-pixel linked-list OIT remains a deferred possible future path. It is not the installed public
alpha mode; use `DVZ_ALPHA_DEPTH_PEEL` for the current explicit higher-quality OIT mode.


## CPU Depth Sort (Optional Source-Over Fallback)

For `DVZ_ALPHA_BLENDED` visuals that request sorted source-over blending, the scene may sort items
by CPU-visible representative depth:

1. item world positions are read from the CPU-side copy of the position buffer,
2. they are sorted by depth (distance from camera) each frame,
3. the sorted index array is uploaded as a draw-order index buffer.

This fallback is O(N log N) per frame and breaks for intersecting geometry.
For static scenes or scenes where items do not intersect, it can produce acceptable results.


## Volume Visuals

Volume visuals (`DVZ_VISUAL_VOLUME`) use fragment shader ray casting and handle their
own internal compositing.
The active first slice supports retained volume visuals with sampled fields, slice/DVR state, and
the normal visual alpha modes. Volumes that opt into `DVZ_ALPHA_WBOIT` contribute transparent
fragments to the WBOIT accumulation path; source-over and depth-peel modes follow their declared
alpha mode.


## Interaction With Item-State Highlight

Item-state visual styles apply to transparent visuals in the same way as opaque visuals.
Alpha styling modifies the per-fragment alpha before OIT accumulation.

Depth-raising or z-bias item-state effects are not meaningful for `DVZ_ALPHA_BLENDED`,
`DVZ_ALPHA_WBOIT`, or `DVZ_ALPHA_DEPTH_PEEL` visuals because normal depth write is disabled in the
transparent pass. If those effects are added later, the scene should diagnose unsupported
transparent-visual combinations.


## FramePlan Structure

For a panel containing both opaque and transparent visuals:

```text
FramePlan (panel):
  RenderNode  — opaque pass  (DVZ_ALPHA_OPAQUE, DVZ_ALPHA_MASK visuals)
  RenderNode  — ordinary transparent pass  (DVZ_ALPHA_BLENDED visuals)
  RenderNode  — WBOIT accumulation pass  (DVZ_ALPHA_WBOIT visuals)
  RenderNode  — OIT resolve pass  (fullscreen composite)
  RenderNode  — depth-peeling passes  (DVZ_ALPHA_DEPTH_PEEL visuals, when present)
  RenderNode  — volume pass  (DVZ_VISUAL_VOLUME visuals)
```

Panels with no WBOIT visuals have no WBOIT accumulation or resolve nodes.
The scene omits unused transparent nodes during frame planning — there is no per-frame overhead
for purely opaque panels.


## Declaring Alpha Mode On A Visual

Alpha mode is a visual-level property, not a per-item property:

```text
dvz_visual_set_alpha_mode(visual, DVZ_ALPHA_WBOIT)
```

It can be changed at any time.
Changing `alpha_mode` marks the visual's render-pass assignment dirty and triggers
a `FramePlan` rebuild for the affected panel.

Per-item alpha is expressed through the item's color alpha channel or an opacity scale.
`alpha_mode` controls the rendering path, not the per-item opacity value.

Blend mode is also a visual-level property:

```text
dvz_visual_set_blend_mode(visual, DVZ_BLEND_ADDITIVE)
```

Changing blend mode invalidates the affected draw pipeline, but it does not create a different
render pass.


## Relationship To Other Documents

| Document | Relationship |
|---|---|
| `pipeline/FRAME_PLAN.md` | render pass split and resolve node in FramePlan |
| `validation/ADAPTATION.md` | exact OIT fallback to weighted OIT; weighted OIT fallback to CPU sort |
| `interaction/SELECTION.md` | highlight alpha multiplier applied before OIT accumulation |
| `semantics/LIGHTING.md` | transparent visuals with PBR shading use the same OIT paths |
| `../semantics/VISUAL_CONTRACT.md` | custom visuals must declare their alpha mode |
