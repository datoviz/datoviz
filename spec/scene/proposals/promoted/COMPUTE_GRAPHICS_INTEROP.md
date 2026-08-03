# Compute And Graphics Interop

Status: promoted implemented experimental v0.4 slice. Updated: 2026-08-03.

The portable slice provides scene buffers with storage/render usage, retained `DvzSceneCompute` work, backend-neutral DRP2 `ResourceBarrier` synchronization, native vklite execution, WebGPU fixture/runtime support, and the `showcases_gpu_particle_smoke` canonical C/WebGPU gallery route. Public documentation labels this surface experimental.

The contract remains deliberately narrow: scene compute lowers through FramePlan and DRP2, does not own windows or backend handles, and does not create a second executable graph. General custom shaders, scheduling, reductions, out-of-core compute, and broad CUDA-owned pointer import are not part of the portable v0.4 contract. Native CUDA/Vulkan external-memory interop remains advanced/unstable and platform-specific.

Current authority lives in the scene and DRP2 public contracts, schemas and fixtures, runtime implementation, `docs/reference/compute-graphics.md`, and the example manifest. Exact release-artifact and physical-platform validation remain release gates rather than unfinished feature implementation.
