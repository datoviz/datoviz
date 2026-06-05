# Showcases

Showcases are curated, composed examples for release proof and the public website. They do not
replace the minimal visual and feature examples.

Current v0.4 gallery-facing examples:

| ID | Source | Status | Notes |
| --- | --- | --- | --- |
| `point_cloud` | `examples/c/showcases/point_cloud.c` | supported source; media pending | Real RESEPI RGB point cloud. Requires `tools/data/prepare_point_cloud.py` and cache-local prepared data. |
| `protein_arcball_viewer` | `examples/c/scientific/protein.c` | supported source; media pending | Real RCSB PDB atom-sphere scene with arcball and postprocess diagnostics. |
| `brain_volume_mesh` | `examples/c/showcases/brain_volume_mesh.c` | supported source; media pending | Allen/IBL RGBA brain volume with occluded slice composition. |
| `showcase_wind_field` | `examples/c/showcases/wind_field.c` | supported source; media pending | Synthetic scalar field, retained vectors, streamlines, and animation. |
| `textured_terrain_or_planet` | `examples/c/showcases/textured_planet.c` | supported source; media pending | Earth/Mars textured mesh proof with capture/video hooks. |
| `showcase_gpu_particle_smoke` | `examples/c/showcases/gpu_particle_smoke.c` | experimental source; media pending | Scene compute-to-render particle smoke proof. |
| `us_state_choropleth` | `examples/c/scientific/choropleth.c` | supported source; Vulkan capture pending | Census state boundary and population-density polygon-set example. |

Before these become final website cards, each item needs a deterministic static screenshot. Animated,
interactive, compute, and 3D-camera examples should also get a short clip.
