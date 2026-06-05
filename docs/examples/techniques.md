# Techniques

Technique examples are planned as focused demonstrations of rendering behavior, separate from visual
families and feature basics.

Current v0.4 coverage is split across feature examples and showcases:

| Technique area | Current source |
| --- | --- |
| Depth testing | `examples/c/features/depth_test.c` |
| Alpha blending | `examples/c/features/alpha_blending.c` |
| Lighting and material response | `examples/c/features/lighting.c`, `examples/c/features/material_mesh.c` |
| EDL pressure | `examples/c/showcases/point_cloud.c` |
| SSAO/MSAA diagnostics | `examples/c/scientific/protein.c` |
| Compute-to-render synchronization | `examples/c/showcases/gpu_particle_smoke.c` |

Dedicated `examples/c/techniques/` sources for WBOIT, MSAA, SSAO, depth cueing, and diagnostic
overlays are not active yet. Legacy technique sources remain under `examples/c/legacy/techniques/`
until they are rewritten as v0.4 C-first examples.
