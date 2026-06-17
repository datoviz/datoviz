# Queries, Picking, and Probing

Queries connect rendered output back to retained scene state. Support is feature-specific, but the
conceptual split between picking, probing, and readback is stable.

## Picking

Picking answers "which scene item is under this pointer or region?" A pick usually starts from a
framebuffer coordinate, maps it to the panel, and resolves it to an item, instance, visual, or
selection target. Picking should report scene-level identities, not raw backend implementation
details.

## Probing

Probing answers "what data value is at this coordinate?" A probe maps from pointer or data
coordinates into an image texel, sampled field, scalar value, or derived readout. Probing depends on
the same transforms, domains, and sampled-resource metadata used for rendering.

## Readback

Readback answers "what did the GPU produce?" It may read pixels, ids, buffers, or query results
from a completed frame. Readback is where backend timing matters most. Browser WebGPU readback is
asynchronous; native paths may also require explicit lifetime and completion handling.

## Result Semantics

The v0.4 rule is to keep query semantics above the backend. User-facing results should be expressed
in terms of panels, visuals, items, samples, coordinates, and retained result state. DRP2 and runtime
readback mechanisms are implementation paths, not the public mental model.

## Support Boundary

Current support is feature-specific. Do not assume every visual family, controller, backend, or
browser route has identical query parity. Use the project and WebGPU status pages for the current
promoted slice.

See also:

- [Pick items](../how-to/pick-and-probe.md)
- [Probe fields](../how-to/probe-fields.md)
- [Queries reference](../reference/queries.md)
- [WebGPU subset](../reference/webgpu-subset.md)
