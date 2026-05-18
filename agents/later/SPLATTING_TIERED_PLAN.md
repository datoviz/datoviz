# Tiered Gaussian Splatting Plan

> **Execution Status**
> - **Status:** `LATER BACKLOG`
> - **Updated on:** `2026-05-18`
> - **Purpose:** preserve tiered splatting strategy without making it an active v0.4 execution
>   entry point.
> - **Current location:** `agents/later/`; this file is not a normative visual or DRP2 contract.

## Summary

Datoviz should add a `splat` visual in v0.4 as a simple screen-space anisotropic Gaussian billboard
primitive, then use v0.5 to strengthen the frame plan and DRP2 layers for proper scalable splatting.
The v0.4 feature should be useful on its own while avoiding architectural dead ends.

The key decision is to avoid a round-only Gaussian billboard visual. Instead, implement anisotropic
splats as the first-class family and treat round Gaussian billboards as the degenerate case where
`sigma.x == sigma.y`.

## Tier 0: v0.4 anisotropic Gaussian splat visual

Goal: add a small, shippable visual family.

### User-visible behavior

- A new `splat` visual renders one screen-facing Gaussian footprint per item.
- Each footprint is an ellipse in screen pixels.
- The isotropic case is expressed by equal x/y sigma values.
- The visual supports per-item position, color, sigma, and eventually angle/opacity.
- The visual participates in normal transparent rendering; WBOIT may be used if already available.

### Minimal attributes

| Attribute | Required | Notes |
|---|---:|---|
| `position` | yes | Center in scene/data coordinates. |
| `color` | yes | Base RGBA color. |
| `sigma` | yes | `vec2f` standard deviations in screen pixels. |
| `angle` | no | Screen-space ellipse orientation. |
| `opacity` | no | Multiplicative alpha. |

### Implementation shape

- Instanced quad billboard rendering.
- Vertex shader projects center and expands by screen-pixel ellipse bounds.
- Fragment shader evaluates Gaussian alpha.
- No compute pass.
- No sorting.
- No indirect draw.
- No projected 3D covariance.
- No spherical harmonic color.
- No 3DGS loader.

### Why this tier is worth doing

- It is substantially more useful than round soft points.
- It provides uncertainty ellipses, anisotropic particle kernels, microscopy point-spread functions,
  directional blobs, and density-like scatter overlays.
- It validates the visual API, shader ABI, alpha/depth routing, and scene-to-DRP2 lowering needed for
  later splat work.
- It avoids making a throwaway round-only visual.

## Tier 1: v0.5 frame-plan readiness for generated GPU workflows

Goal: make the frame plan able to express compute-generated render inputs and richer resource
transitions generically.

### Generic frame-plan additions

- Add explicit access semantics for vertex, index, uniform, and indirect-buffer reads.
- Make executable compute passes first-class graph nodes.
- Support resources written by compute and consumed by render in the same frame.
- Add persistent and transient intermediate resource policies.
- Add better invalidation scopes for camera, viewport, visual data, alpha path, and capability path.
- Add debug JSON fields and diagnostics for generated resource workflows.

### DRP2 additions

- Draw indirect.
- Draw indexed indirect.
- Dispatch indirect.
- Buffer usage flags for generated vertex/index/indirect data.
- Capability-gated validation for compute and indirect features.

### Capability additions

- Compute workgroup limits.
- Storage buffer count and size limits.
- Storage texture support if needed.
- Shader atomic and subgroup support if used by GPU algorithms.
- Float attachment blending and render-target sampling details.
- Indirect draw/dispatch and multi-draw-indirect support.
- Timestamp/profiling support for tuning.

This tier is not splat-specific. It should also benefit GPU aggregation, point-cloud culling,
particle systems, generated mesh instances, and other compute-assisted visuals.

## Tier 2: projected covariance splats

Goal: upgrade from authored screen-space ellipses to camera-dependent Gaussian footprints.

### New data forms

- 2D covariance in screen coordinates.
- 3D covariance in data/world coordinates.
- `scale + rotation` as a compact alternative to full covariance.
- Optional packed/quantized forms for large data.

### Rendering path

- Project 3D covariance into screen-space ellipse per frame.
- Compute conservative bounds.
- Optionally cull splats outside the viewport.
- Reuse the v0.4 anisotropic splat fragment path.

### Open questions

- Which coordinate space owns the covariance: data, world, normalized scene, or view?
- How should covariance behave under nonlinear transforms?
- Should depth be center depth or analytic ellipsoid depth?
- How should projected covariance interact with EDL, SSAO, and volume occlusion?

## Tier 3: scalable transparent splatting

Goal: improve quality and performance for large transparent splat sets.

### Candidate paths

1. Unsorted alpha for sparse or exploratory scenes.
2. Weighted blended OIT as the default quality/performance compromise.
3. CPU depth sorting for moderate item counts.
4. GPU depth sorting for large scenes.
5. Tile-binned splatting for high-end 3DGS-like workloads.

### Required GPU utilities

- Key generation.
- Prefix sum / scan.
- Stream compaction.
- Radix or bitonic sort.
- Tile histogram and offset generation.
- Tile list construction.
- Optional per-tile sorting.

These should be reusable utilities, not one-off splat-only code.

## Tier 4: 3D Gaussian Splatting compatibility

Goal: support 3DGS-like datasets without making Datoviz a neural-rendering-only engine.

### Possible additions

- Spherical harmonic color coefficients.
- Common Gaussian PLY attribute conventions.
- Large persistent splat buffers.
- Streaming and LOD.
- GPU culling, binning, sorting, and indirect draw.
- Viewer examples with Datoviz cameras, panels, picking, and overlays.

### Non-goal

The core Datoviz splat family should remain a scientific visualization primitive. 3DGS dataset
compatibility should be an advanced mode or loader path, not the only definition of splatting.

## Patch sequencing

### Patch A: documentation and specs

- Add `spec/scene/visuals/SPLAT.md` for the v0.4 visual contract.
- Add `spec/scene/proposals/future/SPLATTING_FRAME_PLAN_REQUIREMENTS.md` for v0.5 pressure on frame-plan
  and DRP2 work.
- Add this planning file for tier sequencing.

### Patch B: v0.4 public visual API and retained scene state

- Add the public `splat` constructor and minimal style definitions.
- Add retained data validation for `position`, `color`, `sigma`, optional `angle`, optional `opacity`.
- Add frame-plan visual metadata roles for splat attributes.

### Patch C: v0.4 rendering

- Add splat vertex and fragment shaders.
- Add shader registry and pipeline/technique entries.
- Lower splat visuals to DRP2 render commands.
- Add focused frame-plan and emission tests.
- Add a small example.

### Patch D: v0.5 architecture specs

- Extend frame-plan/pipeline specs for generated GPU workflows.
- Extend DRP2 command/capability specs for compute, indirect, and generated buffers.
- Add fixtures or debug examples for compute-to-render dependencies.

### Patch E: v0.5 implementation series

- Implement richer access semantics.
- Implement executable compute graph passes.
- Implement indirect draw/dispatch support.
- Implement persistent/transient runtime intermediates.
- Add reusable scan/sort/compact/binning utilities as needed.

## Success criteria

### v0.4 success

- Anisotropic splats render as screen-space Gaussian ellipses.
- Isotropic Gaussian billboards require no separate visual.
- The feature uses existing scene and DRP2 render paths.
- The implementation does not introduce v0.5 compute/sorting dependencies.

### v0.5 success

- The frame plan can describe compute-generated render inputs.
- DRP2 can express indirect draw/dispatch when capabilities allow.
- The planner can select and diagnose splat quality paths.
- Advanced splatting can be added without bypassing the frame graph.
