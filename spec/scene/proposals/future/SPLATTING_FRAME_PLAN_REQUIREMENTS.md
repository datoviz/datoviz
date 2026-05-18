> **Execution Status**
> - **Status:** `NON-NORMATIVE FUTURE PRESSURE NOTE`
> - **Updated on:** `2026-05-18`
> - **Purpose:** record v0.5 frame-plan pressure from scalable Gaussian splatting without making
>   those requirements part of the v0.4 splat visual contract.

# Splatting Pressure on the Scene Frame Plan

This document records the generic frame-plan, frame-graph, DRP2, and capability features needed to
move from the v0.4 screen-space anisotropic splat visual to scalable, proper Gaussian splatting. It
is intentionally written as a pressure document: the requested features should be useful beyond
splats, for example for GPU aggregation, particle systems, point-cloud culling, generated mesh
instances, and compute-assisted visualization.

## Baseline assumption

The current v0.4 architecture already has a frame-plan/frame-graph layer. The missing pieces are not
"a frame graph" in the abstract. The missing pieces are richer graph semantics and runtime support for
GPU-generated render inputs, executable compute workflows, sorting/binning algorithms, and expanded
capability reporting.

## Target workflows

### Simple v0.4 splats

```text
visual attributes -> render transparent anisotropic billboard splats -> panel target
```

This path should not require v0.5 frame-plan changes.

### Projected covariance splats

```text
static gaussian attributes
    -> per-frame camera/viewport update
    -> optional compute project/cull pass
    -> render anisotropic screen-space footprints
    -> transparency composite
```

### Proper scalable transparent splatting

```text
static gaussian attributes
    -> compute project covariance and screen bounds
    -> compute cull visible splats
    -> compute generate depth keys or tile bins
    -> sort, compact, or bin
    -> render visible/reordered splats
    -> WBOIT, sorted alpha, or tile-local composite
    -> postprocess and present
```

The frame plan should express these workflows without splat-specific hacks.

## Required generic frame-graph access semantics

The frame graph should distinguish buffer and texture usage precisely enough to validate and emit
correct barriers. In addition to existing sampled/storage/attachment/copy accesses, v0.5 should add
or verify support for:

| Access | Needed for | Example dependency |
|---|---|---|
| `vertex_read` | GPU-generated instance/vertex buffers | compute writes projected splat data, render reads as vertex attributes |
| `index_read` | GPU-generated index or reorder buffers | compute writes sorted indices, render reads indexed draw input |
| `uniform_read` | generated or persistent uniform buffers | upload writes camera parameters, compute/render read them |
| `indirect_read` | GPU-generated draw or dispatch argument buffers | compute writes visible draw count, render uses draw-indirect |
| `storage_read` | compute/render shader storage reads | sort pass reads keys/values |
| `storage_write` | compute/render shader storage writes | cull pass writes visible flags |
| `storage_texture_read/write` | optional tile/composite algorithms | compute writes per-tile image data |

Using `storage_read` as a stand-in for vertex/index/indirect reads is not sufficient long term,
because the runtime must choose correct Vulkan/WebGPU usage flags and barriers.

## Executable compute graph passes

Compute should be first-class in the frame graph rather than only a sidecar command category.
A compute pass descriptor should carry:

1. shader or technique key,
2. dispatch dimensions or indirect-dispatch buffer,
3. declared resource reads and writes,
4. bind group or binding layout information,
5. push constants or small parameter blocks if supported,
6. capability requirements,
7. debug label and diagnostics.

This enables chains such as projection, culling, key generation, scan, sort, compaction, and indirect
argument generation to be validated and emitted as ordinary graph passes.

## GPU-generated draw inputs

Proper splatting needs GPU-generated render inputs. The frame plan and DRP2 should support:

1. buffers written by compute and read as vertex input,
2. buffers written by compute and read as index input,
3. buffers written by compute and read as indirect draw arguments,
4. draw-indirect and draw-indexed-indirect commands,
5. dispatch-indirect commands,
6. optional multi-draw-indirect when capabilities allow.

The initial fallback may still draw all splats and discard invisible items, but the graph should not
make that the only possible architecture.

## Persistent and transient intermediate resources

Advanced splatting uses many intermediate buffers:

1. projected center/radius,
2. screen covariance or ellipse axes,
3. screen bounds,
4. visibility flags,
5. visible indices,
6. depth keys,
7. sorted keys/values,
8. tile counts,
9. tile offsets,
10. tile item lists,
11. indirect draw arguments.

The frame plan should define resource lifetimes and resize policies for:

- borrowed user data,
- persistent visual resources,
- persistent runtime-generated resources,
- per-frame temporaries,
- transient aliasable scratch buffers.

Resource invalidation should distinguish static data changes from camera, viewport, alpha-mode, and
capability/path changes.

## Capability snapshot expansion

The scene capability snapshot and DRP2 capabilities should be rich enough to choose between simple,
compute-assisted, sorted, WBOIT, and tile-binned paths. Useful v0.5 capability fields include:

1. compute support and workgroup limits,
2. storage buffer count and size limits,
3. storage texture support,
4. shader atomic support,
5. subgroup support if used by sort/bin algorithms,
6. 16-bit or packed storage support,
7. float color attachment blending,
8. indirect draw and indirect dispatch support,
9. multi-draw-indirect support,
10. max push constant or small-uniform limits,
11. timestamp/profiling query support,
12. supported floating-point attachment formats for WBOIT,
13. render-target sampling support.

The planner should produce explicit diagnostics when a selected splat quality path is unavailable.

## Sorting, scan, compaction, and binning utilities

Full 3DGS-style splatting should not hide a one-off GPU sort inside the splat visual. v0.5+ should
consider reusable GPU algorithm utilities:

1. prefix sum / scan,
2. stream compaction,
3. radix or bitonic sort,
4. histogram/bin counting,
5. tile offset generation,
6. tile-list filling,
7. optional per-tile local sorting.

These utilities also support large scatter aggregation, point-cloud filtering, GPU-driven mesh
instances, and particle rendering.

## Transparency and compositing paths

The v0.4 splat visual can use ordinary alpha or existing WBOIT routing. Proper splatting needs the
planner to reason explicitly about quality/performance tradeoffs:

| Path | Quality | Cost | Notes |
|---|---|---|---|
| unsorted alpha | low/medium | low | Useful for first demos and sparse splats. |
| weighted blended OIT | medium/good | medium | Good default for scientific transparent clouds. |
| CPU sorted alpha | good for moderate data | medium/high CPU | Useful fallback for small scenes. |
| GPU sorted alpha | high | high | Needs sort/compact/indirect support. |
| tile-binned splatting | high/scalable | very high | Proper 3DGS-like path. |
| depth peeling | high for surfaces | high | May be less suitable for millions of splats. |

The frame plan should expose enough stage information to place splats relative to opaque geometry,
volumes, EDL, SSAO, picking, and final compositing.

## Depth and picking semantics

Splat depth is not a single universal behavior. Future APIs should distinguish:

1. no depth interaction,
2. depth test against opaque geometry only,
3. center-depth contribution,
4. analytic impostor/ellipsoid depth contribution,
5. representative depth for EDL,
6. depth or ID pass for picking.

Picking modes may include nearest center, highest alpha contribution, frontmost center-depth hit, and
top-k contributing splats. GPU request paths should fail explicitly when the selected mode is not
implemented.

## Non-goals for v0.5 frame-plan work

The v0.5 frame-plan upgrade does not need to implement full 3DGS rendering by itself. Its goal is to
make the architecture capable of supporting it cleanly. Dataset loaders, SH color evaluation, and
specific tile-binning kernels can remain later work after the generic frame-plan and DRP2 features
are available.
