# v0.4 Required Showcases

> **Example status:** release showcase bundle
> **Target:** polished native C examples, with optional preprocessing scripts
> **Data:** deterministic synthetic or prepared public/bundled assets
> **Validation:** smoke, screenshot/video capture, interaction checklist, and performance sanity

These are the examples that should carry the v0.4 public story after the small core proofs are
stable.


## `showcases_wind_field`

Primary 2D showcase. It should look like a real field visualization rather than an API fragment.

Current v0.4 implementation target: `examples/c/showcases/wind_field.c`, a synthetic
weather-like scalar field with retained `dvz_vector()` arrows, streamline paths, panzoom,
colorbar-style scale context, deterministic animation, and screenshot/video capture hooks. Broader
projection, coastline, and real weather-data policy remain v0.5/later.


## `showcases_gpu_particle_smoke`

Experimental compute-to-graphics showcase. It should communicate that scene compute can update a
large GPU-resident particle state that normal scene point rendering consumes in the same frame.

Minimal target: one million particles, storage buffers shared by compute and point attributes,
frame-timed simulation parameters, explicit compute-before-render synchronization, transparent smoke
styling, bounded smoke run, and deterministic screenshot capture. Keep it marked experimental until
retained scene compute tasks replace the figure-attached convenience API.


## `showcases_textured_planet`

Required retained textured-mesh proof. This can be a bounded terrain patch, a planet-like surface,
or a narrow Mars/Earth slice, but it must use UVs and real texture sampling.

Current v0.4 implementation target: `examples/c/showcases/textured_planet.c`, a retained
Earth/Mars textured-planet showcase using a UV sphere, mesh-bound sampled RGBA textures,
lighting/material integration, arcball interaction, a procedural star field, and a dated CelesTrak
snapshot of catalogued FENGYUN 1C, IRIDIUM 33, and COSMOS 2251 debris propagated with SGP4. The
prepared two-hour ephemeris drives retained point updates, while closed full-period SGP4 paths show
representative real trajectories in both native and WebGPU routes. Earth, debris, and paths share a
default slow display rotation. A Gaia DR3 bright-star layer and a subdued 2MASS infrared Milky Way
layer remain fixed in the snapshot celestial frame, while a translucent shell suggests Earth's
atmosphere. It covers selected tracked objects, not the full debris environment;
point sizes are exaggerated and do not encode physical size. Baked vertex colors do not satisfy
this scenario.

Mars DEM terrain analysis, registered orthoimage/DEM preprocessing, slope/hazard layers, masks,
probes, and GIS cache policy remain outside this v0.4 slice.


## `showcases_brain_volume`

Narrow neuroscience showcase over the current volume, transparency, occlusion, and controller stack.

Current v0.4 implementation target: `examples/c/showcases/brain_volume.c`, a prepared
Allen/IBL RGBA volume with composite volume rendering, an occluded slice, arcball camera, and
deterministic capture hooks. The cache-local `brain_volume.bin` atlas-mesh preparation is
source material for a later mesh-overlay polish pass. Defer full region picking, atlas trees, and
linked 2D explorer behavior to v0.5.


## `showcases_point_cloud`

Scale/performance showcase for large RGB point clouds with a direct pixel path and lightweight
depth enhancement.

Current v0.4 implementation target: `examples/c/showcases/point_cloud.c`, a dense point-cloud
showcase using the pixel visual, real per-point RGB colors, GUI-tunable EDL, fly-camera
interaction, bounded live loop, and required RESEPI raw-LAZ preprocessing into cache-local prepared
data. It avoids automatic rotation. There is no synthetic or bundled-NPZ fallback; the example fails
with a prep command when real prepared data is missing. LOD and out-of-core policies can follow.
