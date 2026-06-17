# Volume

3D sampled scalar field visual with slice, MIP, and composite rendering modes.

Status: supported.
Backends: native; WebGPU planned (`volume`, `sampled-field`, `texture-3d`, `arcball`).
Primitive: box proxy with raymarching in the native runtime.

## Preview And Links

[![Volume](../../assets/gallery/v0.4/visuals/volume.webp)](../../examples/gallery/visuals/volume.md)

- Example: [Volume](../../examples/gallery/visuals/volume.md)
- How-to: [Use sampled fields and textures](../../how-to/use-sampled-fields.md), [probe image or field values](../../how-to/probe-fields.md)
- Related: [Image](image.md), [Labels](labels.md), [Mesh](mesh.md)

## Use When

Use volume visuals for 3D scalar fields, medical or simulation volumes, and sampled data that needs
slice, MIP, or composited volume rendering.

## Avoid When

Use [Image](image.md) for 2D fields, [Labels](labels.md) for categorical fields, or [Mesh](mesh.md)
for extracted surfaces and triangle geometry.

## Data Model

Create with `dvz_volume(scene, flags)`. Bind a 3D `DvzSampledField` with
`dvz_visual_set_field(volume, "field", field)`, then configure transfer and volume options.

## Attributes

Required: sampled field bound to `"field"`.

Optional: color scale or transfer setup, opacity, sampling mode, render mode, slice axis and
position, raymarch step count, proxy bounds, axis mapping, scalar value range, alpha stops, clipping,
alpha mode, depth test, transform.

## Picking And Probing

Volume visuals are probe-oriented rather than item-oriented. Probe screen positions through the
volume interaction path to recover normalized volume coordinates and sampled values where supported.

## Backend Notes

Native support is active. WebGPU is planned in the manifest because the browser route still needs
3D texture and volume rendering support. The canonical example is native smoke-and-screenshot
validated.

## Canonical Example

- Source: `examples/c/visuals/volume.c`
- Gallery: [Volume](../../examples/gallery/visuals/volume.md)
- Build: `just example-c visuals/volume`
- Smoke: `./build/examples/c/visuals/volume --png`
- Validation: `smoke+screenshot`
- Agent copy-safe: yes

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[use sampled fields](../../how-to/use-sampled-fields.md),
[probe fields](../../how-to/probe-fields.md), [Image](image.md), [Mesh](mesh.md).
