# Datoviz v0.4 Scene Design Notes for Future napari Backend Compatibility

Status: design guidance  
Audience: Datoviz v0.4 scene/runtime developers  
Scope: Datoviz `spec/scene/` implementation decisions that affect future napari integration  
Non-goal: this is not a napari implementation plan and not a proposal to change napari public APIs

---

## 1. Motivation

napari is a model-driven scientific image viewer with a large plugin ecosystem. A future Datoviz backend for napari would be valuable only if Datoviz can act as a **passive, incremental, externally driven rendering backend**.

The desired integration boundary is:

```text
napari ViewerModel / Layer models / plugins
    ↓
napari-datoviz adapter
    ↓
Datoviz Scene / Figure / Panel / Visual / Resource / FramePlan
    ↓
DRP2 command stream
    ↓
Datoviz runtime backend: Vulkan, WebGPU, offscreen, headless, video
````

The key principle is:

```text
napari owns scientific application semantics.
Datoviz owns rendering, GPU resource planning, picking execution, and frame submission.
```

This means Datoviz should not assume that it owns the user-facing data model, application event policy, plugin system, or GUI. It should be possible for an external application such as napari to drive Datoviz scene state from its own retained model.

---

## 2. Current Datoviz v0.4 scene constraints that already fit napari well

The current scene spec is already aligned with napari needs in several important ways.

### 2.1 Scene stays above DRP2

The scene layer:

* owns user-visible visualization state,
* emits backend-agnostic DRP2,
* must not expose Vulkan, swapchain, windowing, allocator, or command-buffer details,
* targets native, browser, offscreen, and headless runtimes through the same semantic layer.

This is essential for napari because napari cannot depend on Vulkan-specific concepts in its layer model or plugin API.

### 2.2 Scene resources are logical and CPU-owned by default

The resource model distinguishes:

* authored CPU-side source resources,
* normalized cached resources,
* transient frame-local derived resources,
* readback resources,
* sampled fields,
* parameter blocks,
* dirty ranges and dirty regions.

This is essential for napari because layers frequently update only part of a texture, table, or parameter set.

### 2.3 FramePlan is the canonical per-frame execution artifact

Datoviz should build one scene-level `FramePlan` per frame. The plan contains uploads, compute nodes, render nodes, copy nodes, readback nodes, logical targets, and dependencies.

This fits napari well because napari interaction changes often require only:

* redraw,
* a small parameter update,
* a dirty texture region,
* a picking readback,

not a complete scene rebuild.

### 2.4 Transform semantics are already separated correctly

The scene spec separates:

```text
DataSpace
    → data-to-visual normalization
VisualSpace
    → panel-local panzoom / camera transform
PanelSpace / ClipSpace
```

This is exactly what napari needs. N-D scientific data, voxel coordinates, physical units, and image pixel coordinates must stay above the GPU backend. Pan/zoom/camera interaction must not force bulk data re-normalization or re-upload.

### 2.5 Picking is scene identity, not backend identity

napari needs picking results in terms of:

* layer,
* item index,
* label value,
* shape index,
* point index,
* mesh face or region,
* sampled value,
* data coordinates.

Datoviz must therefore preserve scene-level identity through batching, draw merging, and GPU readback.

---

## 3. napari concepts that Datoviz developers should keep in mind

A future napari adapter would probably mirror these napari concepts into Datoviz:

| napari concept                    | Datoviz-side concept                                                   |
| --------------------------------- | ---------------------------------------------------------------------- |
| `ViewerModel`                     | external application state driving one or more `DvzFigure`s            |
| `Layer`                           | one or more `DvzVisual`s plus resources and parameter blocks           |
| `Image` layer                     | `image` visual or `volume render_mode=slice`, backed by `SampledField` |
| `Labels` layer                    | integer `SampledField` with categorical lookup and nearest sampling    |
| `Points` layer                    | `point` or `marker` visual with item identity                          |
| `Shapes` layer                    | `path`, `primitive`, or future polygon visual                          |
| `Surface` layer                   | `mesh` visual                                                          |
| `Tracks` layer                    | `path` plus point/marker visual                                        |
| `Vectors` layer                   | `segment` or arrow-like path/marker composition                        |
| napari dims                       | external slicing state feeding sampled fields / texture updates        |
| napari camera                     | external state mapped to panel controller/camera state                 |
| napari colormap / contrast limits | `DvzScale` plus image/volume parameter block                           |
| napari layer opacity/blending     | `alpha_mode`, opacity scale, z-layer, render-pass assignment           |
| napari selection                  | scene selection state or external selection mirrored into styles       |
| napari events                     | external UI/controller events translated into scene state changes      |

The adapter should not create new napari layer types. It should render existing napari layer models.

---

## 4. Non-negotiable design principle for napari compatibility

Datoviz must support this usage pattern:

```text
External app owns model state.
External app mutates Datoviz scene objects.
Datoviz resolves invalidation.
Datoviz builds FramePlan.
Datoviz emits DRP2.
Runtime executes.
Results return as scene-level identities.
```

This implies:

1. Datoviz scene objects must be long-lived.
2. Visuals and resources need stable logical identity.
3. Most updates must be incremental.
4. Panel navigation must not imply bulk upload.
5. Picking must return semantic identity.
6. Runtime/backend details must stay below the scene boundary.
7. External GUI frameworks must remain app-owned clients of scene state.

---

## 5. N-D slicing

### 5.1 napari semantics

napari layers can be N-D:

```text
image.shape = (time, z, y, x)
labels.shape = (time, z, y, x)
points may have coordinates in N dimensions
```

The viewer dims state determines which 2D or 3D slice is displayed. In a napari backend integration, napari should initially remain responsible for N-D slicing.

Recommended integration model:

```text
napari N-D layer data
    → napari dims/slicing/async loader
    → display-ready 2D image, 3D volume, points subset, labels slice
    → napari-datoviz adapter
    → Datoviz SampledField / ItemTable / ParameterBlock update
```

### 5.2 Datoviz design requirements

Datoviz should support:

* full sampled-field replacement,
* subregion sampled-field updates,
* row/element updates for item tables,
* texture updates with explicit row pitch,
* stale update rejection through version counters,
* deferred upload via `UploadNode`,
* no direct GPU call from background threads.

For napari, this is critical because Dask/Zarr-backed images may deliver data asynchronously and out of order.

### 5.3 Recommended Datoviz API pressure

Datoviz should make the following cheap and explicit:

```c
dvz_sampled_field_update(field, data, size);
dvz_sampled_field_update_region(field, x, y, z, w, h, d, data, layout);
dvz_visual_write(visual, attr, offset, count, data);
dvz_visual_write_region(visual, attr, region, data);
```

For large napari datasets, whole-texture replacement is a prototype path only. Serious integration needs dirty regions and tile/chunk updates.

---

## 6. Transforms and coordinate spaces

### 6.1 napari semantics

napari distinguishes, explicitly or implicitly:

```text
data coordinates
layer transforms: scale, translate, rotate, affine
world coordinates
viewer/camera transform
canvas coordinates
```

Datoviz already distinguishes:

```text
DataSpace
    → Stage A: data-to-visual normalization
VisualSpace
    → Stage B: panel-local viewing transform
PanelSpace / NDC
```

This is a strong match.

### 6.2 Required Datoviz behavior

Datoviz should preserve:

1. CPU-side F64 normalization before GPU upload.
2. F32 downcast only after normalization.
3. Separate invalidation for data normalization and panel transforms.
4. Per-panel viewing of the same visual without duplicating data.
5. Per-visual domain overrides.
6. Inverted axes, especially for image-style top-left origins.
7. Explicit pixel-center and voxel-center conventions.

### 6.3 Important convention tests

Add tests specifically for:

```text
1-pixel image at known data coordinate
2x2 checkerboard with known orientation
points overlaid on image pixel centers
labels overlaid on image
inverted Y domain, image y=0 at top
anisotropic voxel spacing
rotated/scaled image with points
2D pan/zoom without resource reupload
```

The most likely integration bugs are not rendering failures. They will be:

* half-pixel offsets,
* flipped Y axes,
* wrong voxel-center convention,
* inconsistent array `(z, y, x)` vs scene `(x, y, z)`,
* unnecessary data upload on pan/zoom.

---

## 7. Layers and visuals

### 7.1 Mapping napari layers to Datoviz visuals

A minimal napari-compatible subset should target:

```text
Image
Labels
Points
```

These cover a large fraction of biological imaging workflows.

Recommended mapping:

| napari layer          | Datoviz implementation target                                    |
| --------------------- | ---------------------------------------------------------------- |
| `Image` 2D scalar     | `image` visual with `texture_mode=scalar`, `DvzScale` colormap   |
| `Image` 2D RGB/RGBA   | `image` visual with `texture_mode=rgba`                          |
| `Image` 3D volume     | `volume` visual                                                  |
| `Image` 3D slice mode | `volume render_mode=slice` or pre-sliced `image`                 |
| `Labels` 2D           | proposed labels path: integer `SampledField` + categorical scale |
| `Labels` 3D slice     | integer `SampledField` slice                                     |
| `Points`              | `point` or `marker` visual                                       |
| `Surface`             | `mesh` visual                                                    |
| `Shapes`              | `path` / `primitive` / future polygon visual                     |
| `Tracks`              | grouped `path` visual plus optional points                       |
| `Vectors`             | `segment` visual                                                 |

### 7.2 Datoviz design requirement

Do not make visual construction require data size up front.

napari frequently creates a layer before all data is loaded, or updates the visible slice dynamically.

Datoviz should support:

```text
create empty visual
attach to panel
later write data
later resize to new item count
later shrink to zero
```

This already matches the v0.4 direction.

---

## 8. Image layer requirements

napari `Image` requires several capabilities already covered by the Datoviz `image` and `volume` specs:

* scalar and RGBA sampled fields,
* colormap scale,
* contrast/domain updates without texture reupload,
* data-aligned image tiles,
* transposed scientific array display,
* axis order/axis flip for volumes,
* slice rendering,
* opacity and blending,
* probe/readout picking.

### 8.1 Datoviz recommendations

For image/volume compatibility, prioritize:

1. `SampledField` as the central abstraction.
2. 2D and 3D sampled fields with explicit semantic hints:

   * scalar,
   * rgba,
   * label,
   * vector,
   * categorical.
3. Partial sampled-field updates.
4. Independent scale/colormap updates.
5. Sampler policy:

   * linear for images,
   * nearest for labels.
6. Axis metadata:

   * dimension order,
   * spacing,
   * origin,
   * unit,
   * axis flip.
7. Readout/probe result payload:

   * scene/data coordinates,
   * sampled value,
   * channel,
   * layer/visual identity.

---

## 9. Labels layer requirements

napari `Labels` is not just an image layer. It has special semantics:

```text
integer label id
0 usually means background / transparent
nearest sampling
categorical color lookup
selected label
paint/edit operations
preserve labels behavior
contour display
label id readout
```

### 9.1 Recommended Datoviz representation

Add or reserve a clear path for a label-field sampled resource:

```text
SampledField semantic = label
component type = uint8 / uint16 / uint32 / int32
sampler = nearest
scale = categorical
background id = 0
background alpha = 0
```

### 9.2 Required Datoviz capabilities for labels

Datoviz should support:

* integer sampled fields,
* unsigned integer texture formats where available,
* categorical color lookup,
* nearest sampling,
* alpha for background label,
* selected-label styling,
* label contour mode,
* partial region updates for painting,
* pick/readout returning label id, not texture coordinate only.

### 9.3 Proposed future visual family or image mode

Two acceptable options:

#### Option A: labels as image variant

```text
image texture_mode = labels
texture = integer SampledField
scale = categorical
```

Advantage: reuses image placement and tiling.

#### Option B: separate `label` visual family

```text
label visual
field = integer SampledField
categorical scale
selection/highlight parameters
paint/update semantics
```

Advantage: makes label-specific semantics explicit.

For napari, Option B is cleaner long-term, but Option A may be faster for an initial prototype.

---

## 10. Points layer requirements

napari `Points` needs:

* per-point position,
* per-point or constant size,
* face color,
* edge color,
* opacity,
* selection,
* visibility,
* point index picking,
* potentially millions of points.

Datoviz `point` already fits much of this, especially because it uses item identity and supports per-item position, color, and size.

### 10.1 Datoviz recommendations

For napari points, prioritize:

1. `point` visual with stable item identity.
2. Streaming position updates.
3. Per-item color and size.
4. Per-group color/size for categorical metadata.
5. GPU picking returning item id.
6. No hardware point sprites; quad/SDF point rendering is preferable for WebGPU compatibility.
7. Efficient selection/highlight through parameter blocks or selection mask, not full color reupload.

---

## 11. Blending, opacity, z order

napari layers have compositing semantics such as:

```text
opaque
translucent
translucent_no_depth
additive
minimum
```

Datoviz currently has `alpha_mode` and transparency pass structure:

```text
DVZ_ALPHA_OPAQUE
DVZ_ALPHA_BLENDED
DVZ_ALPHA_BLENDED_EXACT
DVZ_ALPHA_MASK
```

### 11.1 Recommended Datoviz extension for napari

Add a scene-level compositing mode that is distinct from raw alpha mode:

```text
alpha_mode: how fragments are internally handled
blend_mode: how layer output is composited
z_layer: deterministic 2D layer order
```

Suggested blend modes:

```c
typedef enum
{
    DVZ_BLEND_NORMAL,
    DVZ_BLEND_ADDITIVE,
    DVZ_BLEND_MINIMUM,
    DVZ_BLEND_MAXIMUM,
} DvzBlendMode;
```

Suggested depth policy:

```c
typedef enum
{
    DVZ_DEPTH_DEFAULT,
    DVZ_DEPTH_TEST_WRITE,
    DVZ_DEPTH_TEST_NO_WRITE,
    DVZ_DEPTH_DISABLED,
} DvzDepthMode;
```

For napari 2D, deterministic layer ordering is more important than physically correct depth.

### 11.2 Important implementation rule

Changing opacity should usually update a small parameter block only.

Changing blend mode or alpha mode may require `FramePlanDirty`, because render-pass assignment or pipeline state may change.

---

## 12. Colormaps and scales

napari users expect contrast limits and colormaps to be cheap to update.

Datoviz `Scale` semantics are a good match:

```text
scale domain update
palette update
scalar data update
```

These must remain separate.

### 12.1 Required behavior

For scalar images:

```text
raw scalar sampled field
    → contrast/domain normalization
    → colormap
    → opacity
    → blending
```

Updating the colormap or contrast limits should not force re-upload of the image texture.

### 12.2 Datoviz recommendations

Ensure:

1. `DvzScale` has stable identity.
2. Image, volume, point, mesh, and future label visuals reference scale handles.
3. Scale changes propagate dirty state to dependent visuals.
4. Palette texture updates are small and independent from bulk data.
5. CPU fallback exists when GPU palette lookup is unavailable, with diagnostics.

---

## 13. Picking

napari requires picking for:

* point selection,
* label readout,
* shape selection,
* mesh/surface interaction,
* voxel value probing,
* status-bar coordinate display,
* hover feedback.

Datoviz should preserve scene identity across GPU and CPU picking.

### 13.1 Required result shape

A future Datoviz pick result should be able to express:

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

For napari-specific mappings:

| layer        | picking payload                         |
| ------------ | --------------------------------------- |
| Image        | data coordinates + sampled value        |
| Labels       | data coordinates + integer label id     |
| Points       | point index                             |
| Shapes       | shape index, optional vertex/edge index |
| Surface/Mesh | face index, optional semantic region id |
| Tracks       | track id, point id                      |
| Volume slice | volume id, slice coords, sampled value  |

### 13.2 Async picking

napari hover picking should use latest-request-wins semantics.

Datoviz already specifies this direction. Preserve it.

Required behavior:

```text
hover request 41 issued
hover request 42 issued
result 41 returns late
discard result 41
apply result 42 only if scene/panel generation still matches
```

Click/query picking may use stronger synchronous or blocking semantics.

### 13.3 Avoid backend identity leakage

Never expose:

```text
VkImage
texture coordinate as only authoritative result
draw call id
pipeline id
GPU buffer offset
```

as the final user-facing identity. These may exist internally, but the result must map back to scene identity.

---

## 14. Events and controllers

napari already has its own event model, mouse bindings, Qt interaction, and layer tools.

Therefore, Datoviz must support two interaction modes:

### 14.1 Native Datoviz mode

Datoviz controllers own:

* panzoom,
* camera,
* hover,
* selection,
* linked panels.

### 14.2 External-controller mode

An external application owns interaction and mutates Datoviz state.

For napari, this mode is essential.

Recommended rule:

```text
Datoviz controllers must be optional.
Panels and cameras must be externally settable.
Scene rendering must not require Datoviz-owned event dispatch.
```

Required APIs:

```c
dvz_panel_set_camera_state(panel, ...);
dvz_panel_set_panzoom_state(panel, ...);
dvz_visual_set_visible(visual, bool visible);
dvz_visual_set_opacity(visual, float opacity);
dvz_scene_request_redraw(scene);
```

The napari adapter can then translate napari camera/layer events into Datoviz state updates without handing interaction ownership to Datoviz.

---

## 15. Async data loading and thread safety

napari often uses lazy arrays and background slicing. Background worker threads must not mutate Datoviz scene state or emit DRP2 directly.

Datoviz should preserve the current model:

```text
background thread computes data
    → enqueue transfer
render thread drains transfer queue
    → marks resource dirty
    → invalidation resolved
    → UploadNode appears in FramePlan
    → DRP2 emission
```

### 15.1 Required behavior for napari

The transfer queue should support:

* copy-on-submit mode,
* zero-copy mode with explicit lifetime,
* versioned updates,
* stale update discard,
* back-pressure,
* completion callback when visible on screen,
* callback transfer for structural mutations.

### 15.2 napari slicing-specific recommendation

Add or support update version metadata:

```text
layer_generation
slice_generation
resource_generation
```

Example:

```text
napari requests z=10 → generation 100
napari requests z=11 → generation 101
z=10 returns late
adapter submits generation 100
Datoviz/adapter discards because current generation is 101
```

This avoids flashing old slices during rapid navigation.

---

## 16. Plugin compatibility

napari plugins operate through public napari APIs:

```text
viewer.add_image(...)
viewer.add_labels(...)
viewer.add_points(...)
layer.data = ...
layer.colormap = ...
layer.visible = ...
```

A Datoviz backend must not require plugin authors to call Datoviz APIs.

The adapter must satisfy:

```text
napari plugin creates or mutates standard napari layer
    → napari-datoviz adapter observes the layer
    → Datoviz visual/resource state is updated
    → plugin remains unchanged
```

Compatibility target:

```text
Public napari plugin API: preserve.
Private napari._vispy imports: not guaranteed.
```

This is important politically: the proposal should be framed as replacing or augmenting napari's private rendering backend, not as changing napari's ecosystem.

---

## 17. External UI and Qt embedding

napari is Qt-based. Datoviz should not assume GLFW/ImGui ownership for integration.

The Datoviz scene spec already treats external UI as app-owned. Keep this strong.

For napari, likely presentation modes are:

### 17.1 Native child surface

```text
Qt widget owns native window/surface
Datoviz runtime renders directly into that surface
```

Best performance, hardest integration.

### 17.2 Offscreen texture/image

```text
Datoviz renders offscreen
Qt displays resulting texture/image
```

Easier prototype, slower or more complex for interaction.

### 17.3 Shared GPU texture

```text
Datoviz renders offscreen GPU target
Qt/napari compositor displays shared GPU resource
```

Potentially good long-term, backend/platform-specific.

### 17.4 Recommendation

Datoviz should expose a runtime/render-target abstraction that can support all three without scene-level changes:

```text
DvzRenderTarget canvas target
DvzRenderTarget offscreen target
DvzRenderTarget externally hosted target
```

The scene should never receive a `QWidget`, `VkSurfaceKHR`, `GLFWwindow`, or swapchain handle.

---

## 18. High-DPI and logical pixels

napari and Qt operate with logical pixels and device-pixel ratio.

Datoviz should preserve:

```text
scene coordinates: logical pixels
runtime target allocation: physical pixels
input coordinates: logical pixels
picking coordinates: logical pixels
pixel-unit visual quantities: scaled by DPI before upload
```

This is essential for correct picking and marker/line sizes on Retina/HiDPI displays.

Potential bug class:

```text
Qt sends logical coordinate x=100
Datoviz picking reads physical coordinate x=200
result is wrong by factor dpi_scale
```

The runtime/adapter must handle this consistently.

---

## 19. Datoviz implementation checklist for napari readiness

### 19.1 Object identity

* [ ] Every scene object has stable logical identity.
* [ ] Visual identity is independent from backend object ids.
* [ ] Resource identity is independent from DRP2 ids.
* [ ] Picking maps back to scene identity.
* [ ] Diagnostics mention scene identities, not backend handles.

### 19.2 Incremental updates

* [ ] Buffer subrange dirty tracking.
* [ ] Texture region dirty tracking.
* [ ] Group/span dirty tracking.
* [ ] Parameter-block dirty tracking.
* [ ] Scale/colormap dirty tracking independent from scalar data.
* [ ] Panel transform dirty tracking independent from normalization.

### 19.3 Sampled fields

* [ ] 2D sampled fields.
* [ ] 3D sampled fields.
* [ ] Scalar textures.
* [ ] RGBA textures.
* [ ] Integer/categorical/label fields.
* [ ] Partial region updates.
* [ ] Axis order and axis flip metadata.
* [ ] Nearest and linear sampling policies.
* [ ] Readout/probe metadata.

### 19.4 Visuals

* [ ] Empty visual creation.
* [ ] Resize on data write.
* [ ] Stable item identity.
* [ ] Per-item and constant attributes.
* [ ] Per-group attributes where relevant.
* [ ] Parameter updates without visual recreation.
* [ ] Visibility and opacity setters.
* [ ] z-layer/order setter.
* [ ] alpha mode and blend mode separated.

### 19.5 Picking

* [ ] Pick result includes request id.
* [ ] Pick result includes panel id.
* [ ] Pick result includes visual id.
* [ ] Pick result includes item/group/aux id.
* [ ] Hover latest-request-wins behavior.
* [ ] Stale result discard.
* [ ] Labels return label value.
* [ ] Images/volumes can return sampled value.
* [ ] Points return item index.
* [ ] Meshes return face and optionally semantic region.

### 19.6 Async integration

* [ ] Background threads cannot mutate scene directly.
* [ ] Transfer queue supports data, uniform, callback transfers.
* [ ] Transfer queue supports copy and zero-copy modes.
* [ ] Transfer queue drained before invalidation.
* [ ] Drained transfers become UploadNodes.
* [ ] Stale generation updates can be discarded.

### 19.7 Runtime and UI boundary

* [ ] Scene does not know Vulkan/windowing/swapchain handles.
* [ ] Scene can target canvas, offscreen, and external render targets.
* [ ] External UI can mutate scene state without becoming a scene primitive.
* [ ] Datoviz controllers are optional.
* [ ] External camera/panzoom state can be pushed into panels.
* [ ] High-DPI logical/physical pixel mapping is explicit.

---

## 20. Minimal prototype target for a napari-Datoviz adapter

The first credible prototype should support only:

```text
2D Image
2D Labels
2D Points
pan/zoom
visibility
opacity
colormap/contrast limits
layer order
screenshot
basic picking
```

Avoid initially:

```text
3D volume rendering
all napari blending modes
all shapes
text
tracks
full labels editing
all plugin edge cases
Qt docking complexity
```

A successful first milestone is:

```text
Open napari.
Create standard Image/Labels/Points layers through normal napari APIs.
Render them with Datoviz instead of VisPy.
Show correct pan/zoom, layer order, colormap, opacity, and picking.
Demonstrate performance advantage on large points or large images.
```

---

## 21. Suggested adapter architecture

```python
class NapariDatovizCanvas:
    def __init__(self, viewer):
        self.viewer = viewer
        self.scene = DvzScene()
        self.figure = DvzFigure(...)
        self.panel = DvzPanel(...)
        self.layer_adapters = {}

        self.connect_viewer_events()
        self.sync_existing_layers()

    def connect_viewer_events(self):
        # viewer.layers inserted/removed/reordered
        # viewer.dims changed
        # viewer.camera changed
        # layer events: data, visible, opacity, colormap, contrast_limits, blending
        pass

    def sync_existing_layers(self):
        for layer in self.viewer.layers:
            self.add_layer(layer)

    def add_layer(self, layer):
        adapter = make_layer_adapter(layer, self.scene, self.panel)
        self.layer_adapters[layer] = adapter
        adapter.create_visuals()
        adapter.sync_all()

    def draw(self):
        # apply pending napari state changes
        # drain async updates
        # call Datoviz frame build/submission
        pass
```

Layer adapter shape:

```python
class BaseLayerAdapter:
    def __init__(self, layer, scene, panel):
        self.layer = layer
        self.scene = scene
        self.panel = panel
        self.visuals = []
        self.resources = {}
        self.generation = 0

    def create_visuals(self):
        raise NotImplementedError

    def sync_data(self):
        raise NotImplementedError

    def sync_style(self):
        pass

    def sync_transform(self):
        pass

    def sync_visibility(self):
        pass

    def sync_order(self, z_layer):
        pass

    def destroy(self):
        pass
```

Specific adapters:

```text
ImageLayerAdapter
LabelsLayerAdapter
PointsLayerAdapter
SurfaceLayerAdapter
ShapesLayerAdapter
TracksLayerAdapter
```

The adapter should be thin. Scientific semantics stay in napari. GPU realization stays in Datoviz.

---

## 22. Specific Datoviz API features that would make the adapter simple

These are not final API proposals, but useful pressure points.

```c
/* Scene/resource */
DvzSampledField* dvz_sampled_field_2d(DvzScene*, DvzSampledFieldDesc*);
DvzSampledField* dvz_sampled_field_3d(DvzScene*, DvzSampledFieldDesc*);
void dvz_sampled_field_write(DvzSampledField*, const void* data, DvzWriteDesc*);
void dvz_sampled_field_write_region(DvzSampledField*, DvzRegion3D, const void* data, DvzWriteDesc*);

/* Visual state */
void dvz_visual_set_visible(DvzVisual*, bool visible);
void dvz_visual_set_opacity(DvzVisual*, float opacity);
void dvz_visual_set_z_layer(DvzVisual*, int32_t z_layer);
void dvz_visual_set_alpha_mode(DvzVisual*, DvzAlphaMode mode);
void dvz_visual_set_blend_mode(DvzVisual*, DvzBlendMode mode);

/* External camera/panel state */
void dvz_panel_set_view_2d(DvzPanel*, DvzView2D*);
void dvz_panel_set_view_3d(DvzPanel*, DvzView3D*);
void dvz_panel_set_logical_size(DvzPanel*, uint32_t w, uint32_t h);

/* Picking */
uint64_t dvz_panel_pick_async(DvzPanel*, DvzPickRequest*);
bool dvz_scene_poll_pick_result(DvzScene*, DvzPickResult*);

/* Thread-safe transfer */
void dvz_scene_submit_transfer(DvzScene*, DvzTransfer*);
```

---

## 23. Open design points for Datoviz before napari integration

### 23.1 Labels as first-class semantics

Datoviz should decide whether labels are:

```text
image texture_mode = labels
```

or:

```text
new label visual family
```

The latter is cleaner for napari but more API surface.

### 23.2 Blend modes beyond alpha mode

napari uses blend modes that are not identical to alpha-mode categories. Datoviz probably needs a separate `blend_mode`.

### 23.3 External controller mode

Datoviz should explicitly support panels whose camera/panzoom state is set externally, without using Datoviz controllers.

### 23.4 Render target embedding

napari requires Qt integration. Datoviz should avoid hard assumptions about GLFW. Scene must remain independent; runtime should own the integration details.

### 23.5 Picking latency policy

Click/query can block. Hover should be async latest-wins. Both paths should be supported.

### 23.6 N-D semantics

Datoviz should not try to own arbitrary N-D slicing initially. But `SampledField` metadata should be rich enough that a future Datoviz-side slicing/cache/LOD path is possible.

---

## 24. Recommended phrasing for future napari discussions

Use this framing:

> Datoviz would act as an experimental alternative renderer for napari's existing layer model. The napari public API and plugin ecosystem remain unchanged. The adapter mirrors napari layer state into Datoviz scene resources and visuals, and Datoviz handles GPU resource planning, rendering, picking, and offscreen/readback through its backend-agnostic DRP2 runtime.

Avoid this framing:

> Replace napari with Datoviz.
> Port napari plugins to Datoviz.
> Rewrite napari's layer model.
> Expose Vulkan/WebGPU concepts to napari plugins.

---

## 25. Summary

The Datoviz v0.4 scene spec is already close to what a napari backend would need:

* backend-agnostic scene layer,
* logical resources,
* dirty tracking,
* FramePlan IR,
* explicit transform pipeline,
* scene-level picking,
* thread-safe async handoff,
* external UI boundary,
* high-DPI logical pixel model.

The main implementation priorities for napari readiness are:

1. robust `SampledField` support, including integer label fields,
2. partial updates for textures and buffers,
3. stable scene identities for picking and diagnostics,
4. explicit blend/compositing modes,
5. externally driven camera/panel state,
6. Qt/runtime embedding without scene-level backend leakage,
7. strict separation between N-D slicing semantics and GPU rendering.

If Datoviz preserves these constraints during v0.4 implementation, a napari backend becomes technically plausible without disrupting napari's public API or plugin ecosystem.
