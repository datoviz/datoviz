# Volume

3D sampled scalar field visual with slice, MIP, and composite rendering modes.

Status: supported.
Backends: native; WebGPU planned (`visuals_volume`, `sampled-field`, `texture-3d`, `arcball`).
Primitive: box proxy with raymarching in the native runtime.

## Preview And Links

[![Volume](../../assets/gallery/v0.4/visuals/visuals_volume.webp)](../../examples/gallery/visuals/visuals_volume.md)

- Example: [Volume](../../examples/gallery/visuals/visuals_volume.md)
- How-to: [Use sampled fields and textures](../../how-to/use-sampled-fields.md), [probe image or field values](../../how-to/probe-fields.md)
- Related: [Image](image.md), [Labels](labels.md), [Mesh](mesh.md)

## Use When

Use volume visuals for 3D scalar fields, medical or simulation volumes, and sampled data that needs
slice, MIP, or composited volume rendering.

## Avoid When

Use [Image](image.md) for 2D fields, [Labels](labels.md) for categorical fields, or [Mesh](mesh.md)
for extracted surfaces and triangle geometry.

## Item And Data Model

Create with `dvz_volume(scene, flags)` and bind one scalar 3D `DvzSampledField` to `"field"`. The
constructor generates the 36-vertex box proxy and normalized UVW coordinates internally; user code
should configure bounds rather than upload proxy attributes.

Python's `dvz_sampled_field_from_array()` accepts a C-contiguous scalar array shaped `(D, H, W)`.
Common inferred inputs are `uint8` (`R8_UNORM`) and `float32` (`R32_FLOAT`); specify format,
semantic, color role, and 3D dimension explicitly when inference is not the intended contract.

## Resource And Transfer Contract

| Resource/state | Requirement/default | C/Python representation | Units/coordinates and constraints | Update route |
| --- | --- | --- | --- | --- |
| field slot `"field"` | Required | `DvzSampledField*`; Python sampled-field handle | Scalar 3D field with dimensions `(width,height,depth)` in C or `(D,H,W)` in NumPy. | `dvz_visual_set_field()`; update samples with sampled-field region APIs. |
| color scale slot `"color"` | Optional transfer-color mapping | `DvzScale*`; ctypes handle | Maps normalized scalar values to color. Without an explicit scale, rely only on the generated API's documented field/color-role behavior. | `dvz_visual_set_scale()`. |
| opacity | Default `1` | finite float in `[0,1]` | Global multiplier. | `dvz_volume_set_opacity()`. |
| value range | Default `[0,1]` | two finite doubles, `min < max` | Scalar values mapped to normalized transfer coordinate `[0,1]`. | `dvz_volume_set_value_range()`. |
| alpha stops | Defaults `(0,0)` and `(1,1)` | up to 8 `DvzVolumeAlphaStop` records | Positions and alpha values must be finite in `[0,1]`; input may be unsorted. | `dvz_volume_set_alpha_stops()`. |
| proxy bounds | Default `[-1,-1,-1]` to `[1,1,1]` | two `double[3]`; Python ctypes arrays | Object-space box; each maximum must exceed its minimum. UVW sampling remains normalized. | `dvz_volume_set_bounds()`. |
| axis mapping | Default order `(0,1,2)`, no flips | `uint32_t[3]` permutation plus optional `bool[3]` | Maps normalized volume coordinates to texture UVW. | `dvz_volume_set_axis_mapping()`. |

## Rendering And Clipping Options

- Constructor: `dvz_volume(scene, flags)`; examples pass `flags = 0`.
- Defaults: linear sampling, composite render mode, Z slice at `0.5`, and 64 raymarch steps.
  Sampling supports linear/nearest; render mode supports slice, MIP, and composite.
- Slice position is finite in `[0,1]`; step count must be in `[1,1024]`.
- Axis-aligned clipping uses normalized `clip_min`/`clip_max`. Plane clipping uses a point and
  nonzero normal in normalized volume coordinates plus the side to retain.
- Common visual options include alpha mode, depth test, transform, and color scale.

## Verified Usage Pattern

Create a 3D sampled field, create/bind the volume, configure transfer and bounds, attach it, and use
a 3D camera/controller. The complete
[C and Python example](../../examples/gallery/visuals/visuals_volume.md) demonstrates this route.

## Picking And Probing

Volume visuals are probe-oriented rather than item-oriented. Probe screen positions through the
volume interaction path to recover normalized volume coordinates and sampled values where supported.

## Backend Notes

Native support is active. WebGPU is planned in the manifest because the browser route still needs
3D texture and volume rendering support. The canonical example is native smoke-and-screenshot
validated.

## Canonical Example

| Field | Value |
| --- | --- |
| Source | `examples/c/visuals/volume.c` |
| Gallery | [Volume](../../examples/gallery/visuals/visuals_volume.md) |
| Build | `just example-c visuals/volume` |
| Smoke | `./build/examples/c/visuals/volume --png` |
| Validation | `smoke+screenshot` |

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[use sampled fields](../../how-to/use-sampled-fields.md),
[probe fields](../../how-to/probe-fields.md), [Image](image.md), [Mesh](mesh.md).
