# v0.4 Interaction And Showcase Plan

Status: active cleanup plan. Updated: 2026-06-10.

This plan records the remaining v0.4 examples, interaction, runtime-hardening, and showcase cleanup
work after the current examples/API refactor. It supersedes older pre-implementation notes in this
file: several APIs and examples that were previously planned now exist in the codebase and only need
polish, tests, or data replacement.


## Current Codebase State

Already present in the active tree:

1. public C examples use the v0.4 taxonomy:
   `examples/c/visuals/`, `examples/c/features/`, `examples/c/composites/`,
   `examples/c/showcases/`, `examples/c/advanced/`, and `examples/c/lab/`;
2. `features/orientation_gizmo.c` and the `DvzOrientationGizmo` API exist;
3. `features/reference_grid.c` and the `DvzReferenceGrid` API exist;
4. `features/selection_pixel.c`, `features/selection_sphere.c`, and
   `features/selection_mesh_instances.c` exist;
5. `features/bounds_overlay.c`, `features/gui_viewport.c`, `features/panel_domain_fit.c`,
   `features/triangulation_polygon.c`, and `features/compute_buffer_animation.c` exist;
6. `showcases/embedding_atlas.c`, `showcases/lipid_brain_atlas.c`, and
   `showcases/synthetic_mouse.c` exist, but some data paths still use generated or fallback data;
7. `examples/c/MANIFEST.yaml` contains entries for the active feature and showcase examples.

Do not put large datasets, generated binary payloads, downloaded archives, or derived showcase
artifacts in git. Keep raw data outside the repository or in ignored cache locations. Do not stage
or commit the `data` submodule unless the user explicitly approves it in the current turn.


## Goals

1. Replace public showcase fallback data with real prepared data where agreed.
2. Polish confusing or weak public examples so they teach the intended API clearly.
3. Harden resize and scheduling behavior that affects live examples.
4. Keep v0.4 interaction complete for point-like, pixel, sphere, and instanced mesh items.
5. Keep browser parity as a release-candidate stretch lane, not a blocker unless time allows.
6. Defer box/lasso selection, mesh face selection, full dashboards, and asset import to v0.5.


## Current Agreed Batch From `EXAMPLES_NOTES.md`

This batch records follow-up decisions from the June 2026 examples review.

1. Keep `examples/c/visuals/glyph.c` as a low-level glyph-visual example, but change the content
   from abstract symbols to letters. The preferred implementation is a font-derived atlas prepared
   through the text/font/atlas machinery, then consumed by the raw glyph visual as atlas quads with
   explicit UV bounds. This keeps the example honest about glyph internals while making the output
   recognizable.
2. Add feature examples for the higher-level atlas/text APIs, because the raw glyph visual should
   not be the only public example of atlas-backed rendering:
   - `examples/c/features/text_font_atlas.c`: semantic text using the font/atlas path;
   - optional `examples/c/features/symbol_atlas.c`: atlas-backed symbols distinct from text and
     markers, if the symbol-set API is ready.
3. Use the existing example color identity by default. Prefer the graphite-cyan palette from
   `example_style` for glyph, graph, linked-controller, and 3D showcase polish unless a scene has a
   strong domain-specific color requirement.
4. Replace synthetic/fallback data in the public showcase lane with real prepared data. Processing
   scripts may declare and install Python dependencies such as `pyarrow`, `pandas`, `networkx`, or
   other focused extraction libraries when they are needed. Dependencies must remain processing-time
   dependencies, not Datoviz runtime dependencies.
5. The local lipid source file `/home/cyrille/Downloads/peaks.parquet` may be moved to a durable
   ignored raw cache. It must not be committed.
6. Add a minimal logical layout size of `200x200` for app/view resize handling so very small
   windows clip the already-stable scene instead of driving retained grid/layout machinery into
   invalid geometry. Tests must prove `200x200` works.
7. Keep compute passes one-shot by default. Continuous compute animation must be explicit in the
   example or app scheduling state.


## Bounds Overlay Semantics

`features/bounds_overlay` exposes two different meanings of "bounds" that should not be collapsed.

### Anchor Bounds

Anchor bounds are data/world/visual-space bounds derived from semantic geometry:

1. point, marker, pixel: item positions only;
2. sphere: center positions expanded by sphere radius in data/world units;
3. mesh, primitive, path, image: geometry or declared visual-space extent;
4. visual-local transforms: included because they change visual-space geometry.

Anchor bounds are stable under camera, panzoom, DPI, and output-size changes. They are the right
source for domain fitting, layout diagnostics, data inspection, and most scene-level metadata.
`dvz_visual_bounds()` should keep this meaning unless a separate public API is deliberately added.

### Rendered Bounds

Rendered bounds describe the pixels that a visual may touch after rendering:

1. point/marker/pixel diameter in logical pixels;
2. stroke width, antialiasing, and selection halos;
3. glyph/text atlas quads and font metrics;
4. sphere impostor quad padding and shader antialias padding;
5. camera projection, panel viewport, device scale, user scale, and render scale.

Rendered bounds are panel- and frame-dependent. They are the right source for overlays that promise
to enclose visible marks.

Preferred API shape if this becomes public:

```c
typedef enum DvzRenderedBoundsSpace
{
    DVZ_RENDERED_BOUNDS_PANEL_PX,
    DVZ_RENDERED_BOUNDS_FIGURE_PX,
    DVZ_RENDERED_BOUNDS_FRAMEBUFFER_PX,
} DvzRenderedBoundsSpace;

typedef struct DvzRenderedBoundsDesc
{
    uint32_t struct_size;
    uint32_t flags;
    DvzRenderedBoundsSpace space;
    bool clip_to_panel;
    bool clip_to_plot;
    bool include_antialias_padding;
    bool include_selection_padding;
} DvzRenderedBoundsDesc;

DVZ_EXPORT DvzRenderedBoundsDesc dvz_rendered_bounds_desc(void);

DVZ_EXPORT int dvz_panel_visual_rendered_bounds(
    const DvzPanel* panel,
    const DvzVisual* visual,
    const DvzRenderedBoundsDesc* desc,
    DvzRect* out);

DVZ_EXPORT int dvz_panel_rendered_bounds(
    const DvzPanel* panel,
    const DvzRenderedBoundsDesc* desc,
    DvzRect* out);
```

Do not expose this public API in v0.4 unless another public feature needs it. Implement the
rendered-bounds logic internally first for `bounds_overlay`, because changing public bounds
semantics later would be expensive.

Plan:

1. keep `dvz_visual_bounds()` as anchor bounds;
2. add an internal rendered-bounds or overlay-padding path for bounds overlays;
3. for point/marker/pixel overlays, expand screen-space projected bounds by rendered diameter or
   pixel size;
4. for sphere overlays, account for the same impostor padding used by the sphere shader;
5. add tests proving the visible overlay encloses rendered point and sphere marks;
6. revise `features/bounds_overlay.c` to make the distinction clear, preferably by showing anchor
   bounds and rendered bounds with different line styles if both are exposed.


## GUI Viewport Resize Policy

`features/gui_viewport` still has instability during active ImGui viewport resizing.

The preferred fix is a synchronized resize commit, not a loosely asynchronous in-canvas resize. The
GUI viewport should treat the ImGui content region as a proposed size while the user is dragging,
and commit the offscreen Datoviz resize only when the size has settled for the configured debounce
window.

Policy:

1. while the ImGui window is actively changing size, keep presenting the last valid source frame;
2. do not register a new live-image texture as the visible viewport image until the source resize
   has completed and a frame with the requested framebuffer extent has arrived;
3. if no valid frame exists yet, show an empty reserved region, but do not present it as a valid
   image;
4. hidden or collapsed viewport windows must not churn source resize or render state;
5. keep input forwarding tied to the actually displayed image rectangle.

This is still implemented within the GUI viewport frame loop, but semantically it is a synchronous
resize commit: display changes only after the underlying source target has reached the committed
size.

Plan:

1. track proposed, committed, and displayed source sizes separately;
2. commit source resize only after the proposed size has stabilized;
3. keep the previous displayed image while resize is pending;
4. rebuild the offscreen source, live-image sink, and texture registration at commit time;
5. add a GUI resize-stress test that changes the docked viewport size across several frames and
   verifies no stale, blank, or wrong-extent frame is reported as a valid viewport image.


## Panel Domain Fit Semantics

`features/panel_domain_fit` is not primarily a rendering feature; it is a panel-domain policy. The
current API has only `DVZ_PANEL_VIEW_FIT_CONTAIN` and `DVZ_PANEL_VIEW_ASPECT_EQUAL`.

Semantics:

1. the fit policy owns the X and Y visible data domains;
2. padding expands the source domain before fitting;
3. `DVZ_PANEL_VIEW_ASPECT_EQUAL` expands one domain so one X data unit and one Y data unit occupy
   the same number of plot pixels;
4. the policy is re-applied when the panel plot rectangle changes, including layout reserve and
   figure resize changes;
5. manual pan/zoom can then operate on the resulting visible domain.

For a square-domain example, the content should appear square in the fitted panel. If it does not,
the example or the domain-fit implementation is wrong.

Needed fit-mode decisions:

1. `contain` is the current behavior: preserve all requested data while possibly adding empty space
   on one axis;
2. `cover` may be useful later: preserve equal aspect while filling the plot rectangle, cropping one
   data axis;
3. `stretch/free` is already equivalent to ordinary explicit axis domains without equal aspect;
4. no new fit modes should be added until the `contain + equal` example is visually obvious and
   tested.

Plan:

1. rewrite the example as a clear before/after comparison with a shape where distortion is obvious,
   such as a circle, square, or square grid;
2. show a non-fit panel that stretches the same data into the available plot rectangle;
3. show a fit panel where equal aspect preserves the square/circle and leaves padded empty space;
4. add concise in-scene axis labels or readouts showing the resulting visible domains;
5. add or tighten tests proving equal-aspect fitting preserves unit scale after resize.


## Triangulation Polygon Example

The current polygon shape and styling are visually weak. The example should remain a proof of CPU
polygon triangulation and derived wireframe edges, but it should look intentional.

Plan:

1. use a cleaner polygon, preferably a recognizable concave scientific/geometry shape with one or
   two holes;
2. render a restrained filled polygon surface with low alpha;
3. show the triangulation wireframe clearly, distinguishing boundary edges from interior triangle
   edges;
4. avoid alternating triangle colors unless they teach something specific;
5. keep the CPU `dvz_triangulate_polygon()` and `dvz_geometry_edges()` path as the feature being
   demonstrated;
6. consider a side-by-side layout with input rings on the left and triangulated result plus
   wireframe on the right.


## Glyph, Text, And Atlas Examples

The raw glyph visual should demonstrate atlas-backed quads, but it should not be the only public
example of text/font atlas behavior.

Plan:

1. update `visuals/glyph.c` to render letters from a font-derived atlas while still using
   `dvz_glyph()` directly;
2. add `features/text_font_atlas.c` for semantic text rendered through the font/atlas path;
3. add `features/symbol_atlas.c` only if the symbol-set API is ready enough to avoid churn;
4. keep `visuals/text.c` as the normal semantic text example;
5. document in comments and metadata that glyph is a low-level visual, not the public text API.


## Showcase Data Replacement

### Embedding Atlas

`showcases/embedding_atlas.c` exists but must move from generated/fallback data to a real prepared
dataset.

Prepared bundle target:

1. `xy.f32`: 2D embedding coordinates;
2. `cluster.u16` or `cluster.u32`;
3. `color.rgba8`;
4. `metadata.jsonl`;
5. optional `neighbors.u32` for nearest-neighbor links.

Interaction target:

1. dense point cloud with panzoom;
2. hover metadata readout;
3. click selection;
4. optional nearest-neighbor ring or link segments;
5. optional cluster legend/highlight if the legend/highlight API is stable.

Keep image-thumbnail LOD, text-label LOD, semantic search, and dashboard side panels for v0.5.


### Lipid Brain Atlas

`showcases/lipid_brain_atlas.c` exists, but the current preparation path must extract real data and
the example should move toward a 3D voxel/point-cloud showcase.

Source dataset:

```text
/home/cyrille/Downloads/peaks.parquet
```

The raw Parquet file is very large. It may be moved out of `Downloads` into a durable local cache.
The preprocessing script must:

1. look for an existing cached copy first;
2. if `/home/cyrille/Downloads/peaks.parquet` exists and the cache copy does not, move or copy it
   to the cache location according to a command-line flag;
3. download from Zenodo if no local raw file is available;
4. never place the raw Parquet file in the git repository;
5. write compact render-ready artifacts only to an ignored/generated data directory;
6. install or document processing-time dependencies such as `pyarrow` when needed.

Recommended raw-cache policy:

```text
$DVZ_DATASET_CACHE/lipid_brain_atlas/peaks.parquet
$XDG_CACHE_HOME/datoviz/datasets/lipid_brain_atlas/peaks.parquet
~/.cache/datoviz/datasets/lipid_brain_atlas/peaks.parquet
```

The script should extract a compact real subset. Do not silently fall back to synthetic data when a
raw source is available and readable.

3D target:

1. keep a few representative sections or section ranges;
2. keep a small set of lipid or m/z channels;
3. convert nonzero or thresholded measurements into 3D voxel-center points;
4. store per-point color or scalar values for the selected channel/composite;
5. preserve section/depth metadata so GUI slicing can clip by z range;
6. include optional categorical lipizone/region metadata if present;
7. store min/max or percentile ranges for stable color mapping;
8. store metadata needed for labels, colorbars, and scripted video sweeps.

Showcase behavior:

1. millions-scale colored 3D points/voxels when the compact subset supports it;
2. GUI controls for channel, intensity threshold, z-slice window, opacity, and point size;
3. colorbar and labels;
4. deterministic camera/panel framing;
5. scripted channel or slice sweep for video;
6. optional hover readout if cheap.

Start with point/pixel-like 3D rendering rather than cube meshes. Cube or volume rendering can
follow once the data path and interaction are stable.


### Synthetic Animated Mouse

`showcases/synthetic_mouse.c` exists, but it must not use the old generated ellipsoid/sphere
stand-in for the public showcase.

Source dataset:

```text
https://osf.io/h3ec5/
```

Known source files from the OSF project:

1. `C57BL6_Female_V1.2_opensource-file.blend` for the real mouse mesh/model;
2. `sd3_markers_limb-camera-noise_3d.csv` for real marker coordinates;
3. optional demo-scene `.blend` files for animation provenance if they are useful during
   preprocessing.

Use headless Blender during preprocessing when mesh extraction requires it. Blender is not a
runtime dependency. Datoviz should not load `.blend` files or evaluate Blender rigs at runtime.

Required preprocessed artifacts:

1. static mesh topology;
2. UV coordinates;
3. texture image;
4. normals;
5. baked vertex-position frames or another render-ready animation representation when available;
6. keypoint positions per frame;
7. skeleton edges;
8. trajectory source points, such as body center, nose, and paws.

If full mesh animation is not recoverable from the real source in the first pass, render the
extracted mesh with real marker/skeleton motion and clearly record that mesh deformation is
deferred.

Showcase behavior:

1. animated textured mesh or real mesh plus real skeleton motion;
2. reference grid/floor;
3. orientation gizmo;
4. trajectory trail;
5. current keypoint skeleton overlay;
6. fading keypoint skeleton trail;
7. optional ghost mesh poses.


## Runtime Robustness Fixes

### Small Window Layout

Very small windows must not make retained scene layout fail with diagnostics such as
`scene grid layout resolution failed`.

Implement a minimum logical layout size of `200x200` in the app/view resize path:

1. preserve the actual framebuffer/window size reported by the host;
2. clamp only the logical figure size used for retained layout resolution;
3. let the final scene be clipped when the physical output is smaller than the logical layout;
4. keep `dvz_figure_resize()` itself literal unless a broader public figure-size policy is
   explicitly designed;
5. add tests proving grid/reserve layouts and offscreen rendering work at `200x200`.


### Explicit Continuous Compute

Compute passes should not imply continuous animation by themselves.

Policy:

1. a compute pass is one-shot unless the app, scenario, animation callback, or explicit scheduling
   state requests more frames;
2. `examples/c/features/compute_buffer_animation.c` should make its continuous scheduling explicit;
3. the app scheduler should not infer perpetual animation merely from the presence of compute work;
4. add or update tests so one-shot compute and explicit continuous compute are distinguishable.


## Existing Feature Polish

These features are implemented and should not be treated as greenfield API work:

1. `DvzOrientationGizmo` and `features/orientation_gizmo.c`: polish the visual design toward a
   mesh-arrow/ring passive orientation indicator; keep transform-manipulator behavior out of v0.4.
2. `DvzReferenceGrid` and `features/reference_grid.c`: keep the plane-oriented API; use it in 3D
   showcases where it improves spatial reading.
3. `features/selection_pixel.c`, `features/selection_sphere.c`, and
   `features/selection_mesh_instances.c`: keep focused tests for hit identity, miss behavior,
   hover state, click selection, clear selection, item-state dirtiness, and readback identity.


## Browser Parity Stretch Lane

Do this only if there is time before the release candidate.

Remaining work:

1. verify promoted scenario routes for pixel, sphere, and mesh-instance interaction;
2. verify WGSL query shaders and decode paths for pixel, sphere, and mesh;
3. update WASM scenario host/readback plumbing where native scenarios are ahead;
4. add browser smoke tests and manifest classification;
5. update `docs/reference/webgpu-subset.md`.

Browser parity should not distort the native v0.4 interaction API. If browser support lags, mark the
affected examples as native-first or WebGPU-planned in metadata.


## Implementation Order

1. Fix the small-window `200x200` app/view logical layout clamp and tests.
2. Fix GUI viewport resize as a synchronized committed resize with last-valid-frame display.
3. Clarify bounds overlay semantics and update overlay padding/tests for rendered marks.
4. Rewrite `panel_domain_fit` to visually prove contain/equal-aspect behavior.
5. Redesign `triangulation_polygon` with a cleaner polygon and clearer triangulation wireframe.
6. Update `visuals/glyph` to show font-derived letters through the raw glyph atlas path.
7. Add `features/text_font_atlas.c`; add `features/symbol_atlas.c` only if the symbol API is ready.
8. Make `compute_buffer_animation` use explicit continuous scheduling.
9. Replace generated/fallback embedding, lipid, and mouse showcase data with real prepared data.
10. Polish orientation gizmo and reference grid usage in 3D showcases.
11. Update example manifest/catalog metadata, docs, screenshots, and WebGPU subset docs as needed.
12. Run validation and `git diff --check`.


## Validation

Use the narrowest relevant loop while iterating, then run broader checks before handoff.

Required checks before finalizing code changes:

```sh
just build
just test
git diff --check
```

For each changed public example:

1. build the example;
2. run it through the scenario/example runner if available;
3. capture at least one screenshot or deterministic smoke frame;
4. verify no required asset is missing from the documented cache path;
5. update manifest/catalog metadata;
6. classify browser status as `webgpu-live`, `webgpu-planned`, `webgpu-deferred`, or
   `native-only`.

For interaction features, keep focused tests for:

1. hit identity;
2. miss behavior;
3. hover state;
4. click selection;
5. clear selection;
6. item-state upload dirtiness;
7. selection readback identity.


## Non-Goals For v0.4

Do not include:

1. preserving v0.3 APIs for compatibility;
2. lasso or rectangle selection;
3. mesh face/triangle/vertex picking;
4. Blender runtime loading, skinning, or rig evaluation;
5. image-thumbnail LOD for embeddings;
6. full dashboard side-panel UX;
7. glTF import, which is deferred to v0.5 asset-import work;
8. committing raw datasets or generated binary payloads.
