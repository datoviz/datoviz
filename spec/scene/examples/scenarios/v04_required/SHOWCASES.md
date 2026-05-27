# v0.4 Required Showcases

> **Example status:** release showcase bundle
> **Target:** polished native C examples, with optional preprocessing scripts
> **Data:** deterministic synthetic or prepared public/bundled assets
> **Validation:** smoke, screenshot/video capture, interaction checklist, and performance sanity

These are the examples that should carry the v0.4 public story after the small core proofs are
stable.


## `protein_arcball_viewer`

Flagship current-stack 3D showcase. It should communicate shaded scientific 3D, interaction, and
multi-pass rendering without waiting for full molecular tooling.

Minimal target: prepared protein bundle, atoms as spheres, optional mesh/ribbon or bond fallback,
arcball camera, material/lighting controls, SSAO/MSAA where available, and a bounded screenshot
smoke. Defer full ball-and-stick chemistry, labels, picking, and molecular surfaces if needed.


## `showcase_wind_field`

Primary 2D showcase. It should look like a real field visualization rather than an API fragment.

Minimal target: synthetic climate-like scalar field, arrow field built from primitives until a
vector visual lands, optional streamlines as paths, panzoom, colorbar, and deterministic animation
or capture. Document the primitive-arrow substitution so first-class vector visuals remain visible.


## `textured_terrain_or_planet`

Required retained textured-mesh proof. This can be a bounded terrain patch, a planet-like surface,
or a narrow Mars/Earth slice, but it must use UVs and real texture sampling.

Minimal target: mesh vertices, indices, UV attribute, sampled texture resource, texture color mode,
lighting/material integration, camera path or arcball, and a deterministic capture. Baked vertex
colors do not satisfy this scenario.


## `brain_volume_mesh`

Narrow neuroscience showcase over the current volume, mesh, transparency, and controller stack.

Minimal target: volume slice or MIP with a selected transparent atlas mesh overlay, arcball or
linked 2D/3D view, opacity controls if available, and screenshot proof. Defer full region picking,
atlas trees, and linked 2D explorer behavior to v0.5.


## `dense_point_cloud_edl`

Scale/performance showcase for large point clouds with depth cues.

Minimal target: LiDAR or deterministic dense 3D point cloud, EDL/depth cueing, fly or turntable
camera, bounded live loop, and a capture that makes point-cloud readability visible. LOD and
out-of-core policies can follow.
