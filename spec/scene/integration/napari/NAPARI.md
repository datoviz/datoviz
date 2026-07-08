# Datoviz v0.4 Napari Adapter Pressure Test

Status: informative integration pressure test
Audience: Datoviz scene/runtime developers and future napari adapter authors
Scope: requirements that are specific to rendering napari models with Datoviz v0.4
Non-goal: this is not a napari backend implementation plan and does not define napari APIs

Authority note: this document records napari-specific pressure on the Datoviz scene stack. Use
canonical specs for generic rules:

| Topic | Canonical source |
| ----- | ---------------- |
| Hosted event loops and external surfaces | [../HOSTED_BACKENDS.md](../HOSTED_BACKENDS.md) |
| External UI ownership | [../EXTERNAL_UI.md](../EXTERNAL_UI.md) |
| Logical and physical pixels | [../HIGH_DPI.md](../HIGH_DPI.md) |
| Thread handoff | [../THREAD_SAFETY.md](../THREAD_SAFETY.md) |
| Resource identity and dirty updates | [../../pipeline/RESOURCE_MODEL.md](../../pipeline/RESOURCE_MODEL.md) |
| Frame planning | [../../pipeline/FRAME_PLAN.md](../../pipeline/FRAME_PLAN.md) |
| Coordinate normalization | [../../pipeline/TRANSFORM_PIPELINE.md](../../pipeline/TRANSFORM_PIPELINE.md) |
| Picking and probe readout | [../../interaction/PICKING.md](../../interaction/PICKING.md) |
| Visual families | [../../semantics/VISUAL_FAMILIES.md](../../semantics/VISUAL_FAMILIES.md) |
| Transparency | [../../semantics/TRANSPARENCY.md](../../semantics/TRANSPARENCY.md) |


## Purpose

napari is a model-driven scientific image viewer with a large plugin ecosystem. A Datoviz adapter is
credible only if Datoviz can behave as a passive, incremental renderer for napari's existing model:

```text
napari ViewerModel / Layer models / plugins
    -> napari-datoviz adapter
    -> Datoviz Scene / Figure / Panel / Visual / Resource / FramePlan
    -> DRP2 command stream
    -> Datoviz runtime backend
```

The boundary is the central rule:

```text
napari owns scientific application semantics.
Datoviz owns rendering, GPU resource planning, picking execution, and frame submission.
```

The adapter must not create new napari layer semantics, require plugin authors to call Datoviz APIs,
or expose Vulkan/WebGPU/runtime handles to napari layer code.


## Ownership Boundary

| Area | Owner | Datoviz requirement |
| ---- | ----- | ------------------- |
| `ViewerModel`, layer classes, dims, camera state | napari | Observe and mirror state; do not replace the model. |
| Plugin API calls such as `viewer.add_image()` | napari | Preserve unchanged plugin behavior. |
| Layer transforms, metadata, colormaps, blending names | napari | Translate to Datoviz scene state without losing identity. |
| Visuals, sampled fields, item tables, scales | Datoviz | Keep stable logical identity and incremental dirty state. |
| Frame invalidation and `FramePlan` construction | Datoviz | Resolve updates without full scene rebuilds. |
| DRP2 emission and runtime execution | Datoviz | Keep backend details below the scene boundary. |
| Picking/probe execution | Datoviz | Return napari-meaningful scene identities and values. |
| Qt shell, dock widgets, layer controls | napari | Treat UI as an external host. |

Required usage pattern:

```text
External app owns model state.
External app mutates Datoviz scene objects through the adapter.
Datoviz resolves invalidation.
Datoviz builds FramePlan.
Datoviz emits DRP2.
Runtime executes.
Results return as scene-level identities.
```


## Adapter Shape

The adapter should be thin and replace or augment napari's private rendering backend, not napari's
public layer model.

```text
NapariDatovizCanvas
    owns Datoviz scene/figure/panel/runtime bridge
    observes viewer/layer/dims/camera events
    owns one adapter per napari layer
    submits style/data/order/picking updates to Datoviz
```

Layer adapters should create visuals/resources, sync data/style/transform/order, and destroy Datoviz
logical objects without touching napari layer ownership. Empty creation must be allowed before all
data is loaded.


## Layer Mapping

Minimal napari-compatible pressure should focus on `Image`, `Labels`, and `Points`. These cover many
biological imaging workflows and exercise the hardest shared invariants.

| napari layer | Initial Datoviz target | Key pressure |
| ------------ | ---------------------- | ------------ |
| `Image` 2D scalar | `image` visual + scalar `SampledField` + `DvzScale` | Contrast and colormap updates without data upload. |
| `Image` RGB/RGBA | `image` visual + RGBA sampled field | Correct axis/order/origin handling. |
| `Image` 3D volume | `visuals_volume` visual or sliced `image` | N-D slicing ownership and axis metadata. |
| `Labels` 2D/3D slice | label field path or image-label variant | Integer ids, categorical colors, nearest sampling. |
| `Points` | `point` or `marker` visual | Stable point identity, selection, large counts. |
| `Shapes` | `path`, `primitive`, future polygon visual | Shape identity and edit handles. |
| `Surface` | `mesh` visual | Face/region picking and transforms. |
| `Tracks` | grouped `path` plus optional points | Track id plus point id. |
| `Vectors` | `segment` or arrow-like composition | Per-vector transforms and metadata. |

Datoviz should support creating an attached but empty visual, then later writing, resizing, shrinking,
or clearing its data. napari often creates layer objects before all display data is available.


## Image Requirements

`Image` is the first required layer because it exercises sampled fields, colormaps, coordinates, and
readout.

Required Datoviz behavior:

| Requirement | Reason |
| ----------- | ------ |
| Scalar and RGBA sampled fields | napari supports both scientific scalar and RGB image data. |
| 2D texture and 3D volume inputs | N-D data may be shown as 2D slices or 3D volumes. |
| Partial region/tile updates | Lazy arrays and large images cannot always upload whole textures. |
| Independent scale/colormap updates | Contrast and colormap changes must be cheap. |
| Explicit row pitch and data layout | NumPy slices are not always tightly packed or in display order. |
| Axis metadata: order, spacing, origin, unit, flip | Prevent silent `(z, y, x)` versus `(x, y, z)` bugs. |
| Probe/readout payload | napari needs data coordinates and sampled value. |

For scalar images, keep this separation:

```text
raw scalar sampled field
    -> contrast/domain normalization
    -> colormap
    -> opacity
    -> blend/composite
```

Updating contrast limits, palette, or opacity should not force bulk sampled-field upload.


## Labels Requirements

`Labels` is not just an image with a different colormap. It has layer-specific semantics:

```text
integer label id
0 commonly means background / transparent
nearest sampling
categorical color lookup
selected label
paint/edit region updates
contour display
label-id picking and readout
```

Datoviz needs one clear path for label fields:

| Field property | Required behavior |
| -------------- | ----------------- |
| Component type | `uint8`, `uint16`, `uint32`, or `int32` where supported. |
| Sampling | Nearest, never linear. |
| Scale | Categorical lookup with stable label ids. |
| Background | Configurable background id, usually `0`, with transparent alpha. |
| Updates | Region writes for paint/edit operations. |
| Picking | Return integer label id plus data coordinates. |

Open design choice:

| Option | Shape | Tradeoff |
| ------ | ----- | -------- |
| Image variant | `image texture_mode=labels` | Fastest route; reuses image placement and tiling. |
| First-class visual | `label` visual family | Cleaner semantics for selection, painting, and contour mode. |

For napari, the first-class visual is cleaner long-term; an image-label mode may be acceptable for the
first adapter milestone.


## Points Requirements

`Points` is the first vector-like layer because it pressure-tests item identity, per-item attributes,
large counts, and selection.

Required Datoviz behavior:

| Requirement | Reason |
| ----------- | ------ |
| Stable item id per point | Picking and selection must return napari point indices. |
| Per-item position, size, face color, edge color | napari style can be constant, per-point, or data-driven. |
| Efficient resize/write paths | Points can be appended, removed, or filtered by dims. |
| Selection/highlight without full color reupload | Interactive selection should not rewrite every point. |
| WebGPU-compatible rendering | Prefer quad/SDF marker rendering over hardware point-size reliance. |
| Picking payload | Return point index and, when available, data coordinates. |


## N-D Slicing

napari layers can be N-D:

```text
image.shape = (time, z, y, x)
labels.shape = (time, z, y, x)
points may have coordinates in N dimensions
```

Initial v0.4 adapter rule:

```text
napari owns N-D slicing and async loading.
Datoviz receives display-ready 2D images, 3D volumes, labels slices, or filtered item tables.
```

Recommended flow:

```text
napari N-D data
    -> napari dims/slicing/async loader
    -> display-ready slice/subset
    -> adapter generation check
    -> Datoviz sampled field or item table update
```

Risks to test explicitly:

| Risk | Required mitigation |
| ---- | ------------------- |
| Async slice returns out of order | Use layer/slice/resource generation ids and discard stale updates. |
| Background workers touch rendering state | Submit data to the render thread via the thread-safety boundary. |
| Slice navigation uploads too much | Support region writes and item-table subset writes. |
| Future Datoviz-side slicing desire | Keep sampled-field metadata rich enough for later cache/LOD work. |


## Coordinates And Transforms

napari distinguishes data coordinates, layer transforms, world coordinates, viewer/camera transform,
and canvas coordinates. Datoviz must preserve the equivalent separation described in the transform
pipeline spec:

```text
DataSpace
    -> data-to-visual normalization
VisualSpace
    -> panel-local panzoom / camera transform
PanelSpace / ClipSpace
```

Required behavior:

| Requirement | Why it matters |
| ----------- | -------------- |
| CPU-side F64 normalization before F32 upload | Avoid precision loss for large scientific coordinates. |
| Separate data-normalization and panel-transform invalidation | Pan/zoom must not upload data. |
| Per-panel viewing of the same visual | Linked/multi-view napari layouts must not duplicate resources. |
| Per-visual domain overrides | Layers may have distinct coordinate domains. |
| Explicit pixel/voxel center convention | Prevent half-pixel offsets and wrong readout. |
| Explicit image-origin and axis-flip policy | napari image y=0 often maps to top of display. |

Convention tests:

```text
1-pixel image at known data coordinate
2x2 checkerboard with known orientation
points overlaid on image pixel centers
labels overlaid on image
image y=0 at top
anisotropic voxel spacing
rotated/scaled image with points
2D pan/zoom without resource reupload
```


## Blending And Layer Order

napari layer compositing names are not identical to low-level alpha modes:

```text
opaque
translucent
translucent_no_depth
additive
minimum
```

Datoviz should keep separate concepts:

| Concept | Meaning |
| ------- | ------- |
| `alpha_mode` | How fragments are internally handled: opaque, blended, WBOIT, depth peel, mask. |
| `blend_mode` | How layer output is composited: normal, additive, minimum, maximum. |
| `z_layer` | Deterministic 2D layer order. |
| `depth_mode` | Depth test/write policy for 2D/3D interaction. |

Changing opacity should usually update a small parameter block. Changing blend mode, alpha mode, or
depth policy may require frame-plan or pipeline-state invalidation.


## Picking And Readout

napari needs picking for point selection, label readout, shape selection, mesh interaction, voxel
probing, status-bar coordinates, and hover feedback. Datoviz must return scene identities, not backend
implementation ids.

Required result shape:

```text
request_id
panel_id
visual_id
family_id
item_id
group_id
auxiliary_id
data_coordinates
sampled_value
valid / stale / missed
```

Layer-specific payloads:

| Layer | Payload |
| ----- | ------- |
| `Image` | Data coordinates and sampled value. |
| `Labels` | Data coordinates and integer label id. |
| `Points` | Point index and optional data coordinates. |
| `Shapes` | Shape index and optional vertex/edge index. |
| `Surface` / mesh | Face index and optional semantic region id. |
| `Tracks` | Track id and point id. |
| Volume slice | Volume id, slice coordinates, sampled value. |

Hover picking should use latest-request-wins semantics:

```text
hover request 41 issued
hover request 42 issued
result 41 returns late
discard result 41
apply result 42 only if scene/panel generation still matches
```

Do not expose `VkImage`, draw call id, pipeline id, GPU buffer offset, or raw texture coordinate as
the final user-facing identity.


## External Control

napari owns events, mouse bindings, Qt tools, and layer controls. Therefore Datoviz controllers must
be optional for hosted use.

Required adapter-facing capabilities: externally set panel camera/panzoom state, update layer
visibility/opacity/order without visual recreation, request redraws, produce napari-compatible
screenshots, and render to external/offscreen/canvas-backed targets without scene changes.

The scene layer should never receive a `QWidget`, `VkSurfaceKHR`, `GLFWwindow`, or swapchain handle.


## Plugin Compatibility

The adapter must preserve normal napari plugin behavior:

```text
plugin calls viewer.add_image(...)
plugin mutates layer.data / layer.visible / layer.opacity / layer.colormap
adapter observes standard napari state
Datoviz visuals/resources update
plugin remains unchanged
```

Compatibility target:

```text
Public napari plugin API: preserve.
Private napari._vispy imports: not guaranteed.
```

Recommended framing:

```text
Datoviz is an experimental alternative renderer for napari's existing layer model.
napari public APIs and plugin semantics remain unchanged.
```

Avoid framing the work as replacing napari, porting plugins to Datoviz, or exposing Datoviz backend
concepts to plugin authors.


## Minimal Credible v0.4 Milestone

The first credible v0.4 milestone should support:

```text
2D Image
2D Labels
2D Points
pan/zoom
visibility
opacity
colormap/contrast limits
deterministic layer order
screenshot
basic picking/probe readout
```

Acceptance criteria:

| Criterion | Expected result |
| --------- | --------------- |
| Normal napari APIs | Standard `viewer.add_image`, `viewer.add_labels`, and `viewer.add_points` render through Datoviz. |
| Correct coordinates | Image, labels, and points align under pan/zoom and probe coordinates are meaningful. |
| Correct styling | Visibility, opacity, colormap, contrast limits, and layer order update incrementally. |
| Correct identity | Picking returns label ids, point indices, and image sampled values. |
| Incremental updates | Pan/zoom and style changes do not re-upload bulk data. |
| Performance pressure | Large images or large point sets show a reason to pursue the backend. |

Explicit non-goals for the first milestone: all napari blending modes, 3D volume rendering parity,
full labels editing, all Shapes/Tracks/Vectors behavior, text rendering, all plugin edge cases, and
Qt docking/runtime polish.


## Open Datoviz Decisions

| Decision | Why it matters for napari |
| -------- | ------------------------- |
| Labels representation | Choose image-label variant, first-class label visual, or both staged over time. |
| Blend mode API | napari blend modes require a layer compositing concept separate from alpha handling. |
| External controller mode | Panels need an explicit externally driven camera/panzoom path. |
| Hosted render targets | Qt integration needs direct, offscreen, or shared-GPU presentation without scene leakage. |
| Picking latency policy | Hover should be async latest-wins; click/query may need stronger semantics. |
| N-D ownership line | napari owns initial slicing, but Datoviz metadata should not block future cache/LOD work. |
| Label edit/update path | Painting requires small region writes and stale-update rejection. |
| Selection/highlight representation | Points and labels need selection state without full attribute rewrites. |


## Summary

Datoviz v0.4 is a plausible napari renderer if it preserves these constraints:

1. napari owns model, plugin, UI, dims, and scientific semantics.
2. Datoviz owns scene realization, planning, resource updates, rendering, and picking execution.
3. Image, Labels, and Points work first, with stable identities and incremental updates.
4. N-D slicing, coordinates, blending, and picking are treated as integration risks, not afterthoughts.
5. Generic hosted-backend, high-DPI, threading, resource, and picking rules stay in canonical specs.
