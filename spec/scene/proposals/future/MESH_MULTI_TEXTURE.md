# Mesh Multi-Texture Overlay

Status: v0.5 feature request.

The mesh visual currently supports one texture slot (diffuse/albedo). This proposal adds support
for binding multiple independent textures to a single mesh visual, enabling scientific data overlays
on geometric surfaces.


## Motivation

A common scientific visualization pattern is a geometric surface carrying multiple independent data
fields simultaneously. Representative use cases:

- **Earth/climate**: satellite base texture + temperature anomaly colormap + sea ice mask
- **Medical imaging**: MRI surface mesh + functional activation map + parcellation atlas
- **Material science**: surface mesh + stress field + temperature field
- **Astronomy**: planet texture + atmospheric density overlay

These cannot be expressed with the current single-texture mesh without CPU-side compositing, which
loses the ability to update fields independently and limits interactivity.


## Proposed API

Add a slot index to the existing texture setter:

```c
dvz_visual_set_texture_slot(visual, uint32_t slot, DvzId texture_id);
```

Slot 0 is the existing diffuse texture (backwards compatible). Slots 1 and 2 are new data overlay
slots. Each slot has an independent sampler.

Also expose a per-slot blend mode:

```c
dvz_visual_set_texture_blend(visual, uint32_t slot, DvzTextureBlend mode);
```

Blend modes:

| Mode | Meaning |
| --- | --- |
| `DVZ_TEXTURE_BLEND_REPLACE` | Slot 0 only; replaces base color entirely. |
| `DVZ_TEXTURE_BLEND_ALPHA` | Standard alpha compositing over the layer below. |
| `DVZ_TEXTURE_BLEND_ADD` | Additive blend; useful for emission or highlight overlays. |
| `DVZ_TEXTURE_BLEND_MULTIPLY` | Multiply blend; useful for masks and modulation. |
| `DVZ_TEXTURE_BLEND_MIX` | Linear mix controlled by the texture alpha channel. |


## Shader Changes

Add two optional texture/sampler pairs to the mesh fragment shader descriptor set 1:

```glsl
layout(set = 1, binding = 3) uniform texture2D overlay_tex_1;
layout(set = 1, binding = 4) uniform sampler overlay_samp_1;
layout(set = 1, binding = 5) uniform texture2D overlay_tex_2;
layout(set = 1, binding = 6) uniform sampler overlay_samp_2;
```

Slots are optional — if not bound, the shader skips that layer. A small uniform block carries the
active slot mask and per-slot blend mode.

The WGSL port should follow the same layout.


## Design Constraints

- Backwards compatible: existing single-texture mesh code requires no changes.
- Slot count capped at 3 (base + 2 overlays) to stay within typical WebGPU binding limits.
- All overlay slots share the same `texcoords` attribute as the base texture. Independent UV sets
  per slot are out of scope for this version.
- Instanced mesh and item_state styling remain incompatible with textured mesh (existing constraint
  unchanged).


## Relation to PBR Roadmap

The reserved `normal_map` slot from the PBR upgrade path (MESH_SHADING_DESIGN.md) occupies binding
3 in the current plan. Coordinate with that proposal to avoid descriptor set conflicts before
implementation.


## Effort Estimate

2–3 weeks: shader changes, lowering updates, API additions, WGSL port, one example
(earth + wind/temperature overlay).
