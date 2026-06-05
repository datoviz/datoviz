# Validation Gallery

This page will collect release evidence: screenshots, videos, fixture dashboards, and commands that
prove the declared v0.4 feature set.

Current evidence sources:

| Evidence | Location |
| --- | --- |
| Native C examples | `examples/c/MANIFEST.yaml` and `examples/c/` |
| WebGPU fixture dashboard | `examples/webgpu/fixtures.html` |
| WebGPU compatibility log | `examples/webgpu/COMPAT.md` |
| WebGPU public subset notes | `docs/reference/webgpu-subset.md` |
| Compute-to-render proof | `examples/c/showcases/gpu_particle_smoke.c` and `docs/reference/compute-graphics.md` |

Missing before final gallery publication:

1. deterministic screenshots for each public visual, feature, workflow, showcase, and scientific
   example;
2. short clips for animated, interactive, 3D-camera, and compute examples;
3. a generated gallery index backed by `examples/c/MANIFEST.yaml`;
4. recorded Vulkan-capable capture proof for examples that cannot render in headless CI.
