# Coordinate Systems

Datoviz separates semantic coordinates from backend coordinates. User code should describe data in
the coordinate system that makes sense for the visualization; frame planning then lowers that state
to the GPU-facing spaces required for rendering.

The common spaces are:

- data coordinates: the user's scientific or application coordinates;
- panel coordinates: the visible domain attached to a panel;
- world/view coordinates: the camera or controller-oriented representation used by 3D navigation;
- clip coordinates: the normalized GPU space after projection;
- framebuffer coordinates: pixel positions in the render target;
- texture or sample coordinates: indices or normalized coordinates used for images and fields.

For v0.4, semantic and domain coordinates should remain authoritative in double precision where the
scene contract needs it. Visual render attributes are lowered to GPU-facing float data unless a
family contract says otherwise. This keeps the public model precise without requiring every shader
input to be double precision.

Controllers change transforms, not the original data. Panzoom changes the visible 2D domain.
Arcball, fly, turntable, and orbit controllers change view or camera state for 3D panels. Linked
panels should share controller or domain state explicitly.

Nonlinear or geographic projections are not scene-managed in v0.4. The supported pattern is to
project data on the CPU into ordinary Cartesian coordinates, then upload the projected positions to
Datoviz. A future scene-managed projection system should preserve the same distinction between data
semantics and GPU lowering.

Picking and probing depend on the same coordinate chain as rendering. A pointer starts in
framebuffer coordinates, maps through the panel viewport into panel or data coordinates, and may
then map into an item id, image texel, field sample, or readback request.

See also:

- [Coordinate systems reference](../reference/coordinate-systems.md)
- [Use coordinate systems](../how-to/coordinate-systems.md)
- [Query, pick, and probe model](query-pick-probe-model.md)
