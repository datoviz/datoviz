# Example Coverage Plan

Datoviz v0.4 documentation uses examples as executable release proof. Every public visual and
public feature should have a focused C example that demonstrates only that visual or feature and the
minimum surrounding setup needed to run it.

The mapping is not `one feature = one how-to page`. The required unit is the executable example.
How-to pages group common user tasks across examples, and tutorials compose multiple examples into
narrative workflows.

The aspirational gallery direction is recorded in
[`../scene/examples/PLANNING.md`](../scene/examples/PLANNING.md). Use that
document to choose screenshot, video, and showcase targets before reducing them to concrete release
fixtures and current implementation work.


## Example Principles

1. One public visual family gets one minimal C example.
2. One public feature gets one minimal C example.
3. A minimal example should avoid unrelated visual polish.
4. A showcase may compose many features, but it does not satisfy the minimal-example requirement.
5. Every example should be linked from the relevant reference and how-to pages.
6. Examples should have stable identifiers that can be used by docs, tests, release notes, and LLM
   retrieval.
7. Copy-safe examples should use the preferred scene/app ownership pattern and declare their
   validation command.
8. If an example is only an API sketch, pressure test, or showcase, mark it so agents do not copy it
   as a minimal starting point.
9. How-to pages should not be created one-to-one for every feature. Add a how-to page when a user
   task needs workflow guidance, tradeoffs, ownership notes, or adaptation advice.
10. Visual documentation should include authored judgment such as "use when", "avoid when", and
    "choose this over that"; keep generated or mechanical facts in reference tables.


## v0.4 Coverage Contract

The v0.4 example set is allowed to replace, rename, split, or delete old v0.3 examples. Old gallery
pages and legacy examples are provenance only; they are not compatibility requirements.

Use these rules when implementing or reviewing a minimal example:

1. Visual examples teach the shape of one visual family's data and the smallest useful styling
   attributes for that family.
2. Feature examples may use one plain visual as scaffolding, but the screenshot should make the
   feature under test more obvious than the scaffolding visual.
3. Runtime and technique examples should be separate from visual-family coverage unless the runtime
   behavior is inseparable from the visual proof.
4. Showcases can be polished and composed, but they do not satisfy one-visual or one-feature
   coverage.
5. Minimal examples should be deterministic, C-first, scene/app-first, and copy-safe once the API is
   stable.
6. Do not keep a weak example just because it existed in v0.3. Prefer a small rewrite that teaches
   the v0.4 API cleanly.
7. A visual or feature row is not complete until the expected rendered result, source path,
   validation command, screenshot policy, and copy-safe status are known.

Recommended implementation states:

| State | Meaning |
| --- | --- |
| `planned` | Desired v0.4 example shape is specified, but source may not exist yet. |
| `candidate` | A source file exists or legacy source can be salvaged, but docs/capture may need rewrite. |
| `ready-now` | Current source can be polished without waiting for release blockers. |
| `needs-rc1-proof` | Feature exists, but needs a runnable example, capture, fixture, or validation pass. |
| `experimental` | In the v0.4 public experimental subset; docs and metadata must label it honestly. |
| `conditional` | Include only if the visual or feature remains public for v0.4. |
| `deferred` | Do not force into v0.4 minimal coverage. |
| `external/GSP` | Belongs primarily outside Datoviz C examples. |


## Documentation Pairing

Every public visual or feature needs at least one minimal executable example. The prose around that
example depends on what readers need:

| Page type | Role | Relationship to examples |
| --- | --- | --- |
| Example | Smallest runnable proof | One public visual or feature should have at least one |
| Reference | Exact facts, status, attributes, limits | Links to minimal examples and may be generated in part |
| How-to | Task workflow and adaptation guidance | Groups several related examples when useful |
| Tutorial | Narrative learning path | Composes a stable set of examples and concepts |

Avoid writing polished tutorial prose before the underlying example is stable. It is acceptable to
write scope, status, ownership, and "choose the right visual" prose earlier because that guidance
prevents users and agents from overpromising unstable behavior.


## Suggested Source Layout

```text
examples/c/
  visuals/
  features/
  techniques/
  showcases/
  runtime/
  drp2/
```

WebGPU examples and fixtures may keep their existing browser-oriented layout under `examples/webgpu/`
while being indexed by the same documentation manifest.


## Visual Examples

Required dedicated visual examples are below. Each row describes the intended v0.4 public example,
not the old gallery page that may currently occupy a similar slot.

| ID | Source | State | Expected rendered result | Teaches and limits |
| --- | --- | --- | --- | --- |
| `visual.point` | `examples/c/visuals/point.c` | `planned` | A deterministic 2D grid or compact cloud of points with visible per-point color and size variation. | Teaches retained point positions, colors, and sizes. Avoid axes, picking, animation, and complex colormaps. |
| `visual.pixel` | `examples/c/visuals/pixel.c` | `planned` | A dense raster-like grid of square pixels, with clear cell boundaries or value variation at thumbnail size. | Teaches pixel positions, sizes, and color/value mapping. Avoid image texture APIs and probe readouts. |
| `visual.marker` | `examples/c/visuals/marker.c` | `planned` | A small arranged set of marker shapes large enough to inspect, with shape, fill/stroke color, and size variation. | Teaches marker shape and styling attributes. Put picking in `feature.pick_marker`, not here. |
| `visual.primitive` | `examples/c/visuals/primitive.c` | `planned` | A few simple primitives arranged so topology differences are obvious, preferably triangles or quads with distinct colors. | Teaches topology-parametric primitive data. Avoid shape builders, polygons, and mesh indexing. |
| `visual.segment` | `examples/c/visuals/segment.c` | `planned` | Independent line segments with varying color and width, visibly not connected into a continuous path. | Teaches endpoint-pair data and per-segment styling. Avoid path joins, axes, and vector arrowheads. |
| `visual.path` | `examples/c/visuals/path.c` | `planned` | One or more continuous polylines, such as deterministic signals or smooth curves, with clear stroke styling. | Teaches ordered path data, path breaks, width, and color. Avoid axes unless implementing `feature.axes_2d`. |
| `visual.vector` | `examples/c/visuals/vector.c` | `planned` | A small quiver or flow field where direction, length, and optional arrowheads are readable. | Teaches vector anchors, vector values, scale, and color. Avoid wind-field showcase polish and animation. |
| `visual.image` | `examples/c/visuals/image.c` | `planned` | A scalar 2D field or RGB image filling most of the panel, with enough structure to see orientation and sampling. | Teaches image or 2D sampled-field upload. Put colorbar and probing in feature examples. |
| `visual.mesh` | `examples/c/visuals/mesh.c` | `planned` | A simple indexed 3D surface with readable shape, normals or flat colors, and a stable camera pose. | Teaches vertices, indices, mesh creation, and basic 3D viewing. Put texture in `feature.mesh_texture`. |
| `visual.sphere` | `examples/c/visuals/sphere.c` | `planned` | A small 3D cluster of impostor spheres with depth, radius, and color variation visible. | Teaches sphere centers, radii, colors, and depth behavior. Avoid molecule-showcase semantics. |
| `visual.volume` | `examples/c/visuals/volume.c` | `planned` | A compact 3D volume rendering or slice stack with clear internal structure and deterministic transfer defaults. | Teaches 3D sampled-field or volume visual setup. Avoid full medical-viewer controls. |
| `visual.text` | `examples/c/visuals/text.c` | `conditional` | A few short strings placed in data and/or screen space with legible anchors, color, and size. | Teaches semantic text creation if text is public. Do not expose internal glyph visual details. |
| `visual.labels` | `examples/c/visuals/labels.c` | `conditional` | An integer label field or labeled regions with distinct categorical colors and stable ids. | Teaches label-field upload and categorical styling if labels are public. Put probe/readout in `feature.probe_labels`. |
| `visual.splat` | `examples/c/visuals/splat.c` | `experimental` | A small splat cloud with obvious footprint, color, and depth blending behavior. | Public experimental coverage only. Keep full Gaussian-splat asset pipelines out of v0.4 coverage. |
| `visual.errorbar` | `examples/c/visuals/errorbar.c` | `deferred` | A minimal uncertainty glyph around points if error bars become a first-class visual. | Do not force into v0.4 unless the visual contract is promoted. |
| `visual.boxplot` | `examples/c/visuals/boxplot.c` | `deferred` | A compact statistical distribution plot if boxplots become first-class. | Likely belongs in GSP/plot unless Datoviz exposes a low-level visual. |
| `visual.tube` | `examples/c/visuals/tube.c` | `deferred` | A 3D tubular path with lighting and radius variation if tubes become public. | Do not replace the segment/path/vector minimal examples. |

Adornments and scene features such as axes, colorbars, scale bars, annotations, overlays, and
controllers belong under feature examples, not visual examples, unless the public API explicitly
presents them as visual families.

The old `basic`, `glyph`, and `wiggle` gallery pages should not define v0.4 minimal coverage. Keep
`glyph` as an internal lowering detail unless a public glyph API is explicitly exposed.

Current implementation seeds:

| ID | Useful existing source or page | Migration note |
| --- | --- | --- |
| `visual.point` | `examples/c/legacy/visuals/point.c`, `docs/gallery/visuals/point.md` | Salvage the visual idea; rewrite as C-first `point_2d` retained-scene smoke. |
| `visual.pixel` | `examples/c/legacy/visuals/pixel.c`, `docs/gallery/visuals/pixel.md` | Salvage dense grid/raster idea; keep image upload and probing elsewhere. |
| `visual.marker` | `examples/c/features/pick_marker.c`, `docs/gallery/visuals/marker.md` | Use `pick_marker.c` only as API evidence; write a non-picking marker baseline. |
| `visual.primitive` | `examples/c/legacy/visuals/primitive.c`, `docs/gallery/visuals/basic.md` | Replace the old `basic` page with `primitive`; there is no v0.4 `basic` visual family. |
| `visual.segment` | `examples/c/legacy/visuals/segment.c`, `docs/gallery/visuals/segment.md` | Salvage endpoint/stroke examples; keep paths and vectors separate. |
| `visual.path` | `examples/c/legacy/visuals/path.c`, `docs/gallery/visuals/path.md` | Salvage curve/signal composition; axes belong in `feature.axes_2d`. |
| `visual.vector` | `examples/c/legacy/visuals/vector.c`, `examples/c/showcases/wind_field.c` | Required near-term because wind/flow examples should stop using ad hoc primitives. |
| `visual.image` | `examples/c/legacy/visuals/image.c`, `examples/c/features/image_probe.c` | Extract a plain image baseline; leave probing in `image_probe` and colorbar/readout behavior in their own feature rows. |
| `visual.mesh` | `examples/c/legacy/visuals/mesh.c`, `docs/gallery/visuals/mesh.md` | Salvage as lit/indexed mesh; textured mesh is a separate feature/showcase proof. |
| `visual.sphere` | `examples/c/legacy/visuals/sphere.c`, `examples/c/scientific/protein.c` | Salvage impostor-sphere proof; molecule/protein remains scientific material. |
| `visual.volume` | `examples/c/legacy/visuals/volume.c`, `docs/gallery/visuals/volume.md` | Salvage volume setup; keep full brain/medical composition as showcase. |
| `visual.text` | `examples/c/legacy/visuals/text.c`, `examples/c/lab/text_msdf_diagnostics.c` | Salvage only through the public text API; do not document raw glyph internals. |
| `visual.labels` | `examples/c/legacy/showcase/labels.c` | Promote only if labels remain public; split probing into `feature.probe_labels`. |
| `visual.splat` | `examples/c/legacy/showcase/gothic_splat.c` | Experimental v0.4 visual; publish with explicit experimental labeling and no full asset-pipeline promise. |
| `composite.polygon` | `examples/c/legacy/visuals/polygon.c`, `docs/gallery/features/polygon.md` | Polygon is in v0.4 release scope as a semantic composite; rebuild the old feature page as a C-first composite page. |


## AI-Assisted Coverage Matrix

The table below records the minimum example roles that make common generated-code requests safe.
`Minimal` examples should be copy-safe by default. `Update`, `offscreen`, and query examples may
live under `features/` or `runtime/` when they are shared across visual families.

| Visual family | Minimal | Update or streaming | Offscreen/capture | Query or interaction |
| --- | --- | --- | --- | --- |
| Point | required | required | required | point picking |
| Pixel | required | desired | desired | hover/readout if exposed |
| Marker | required | desired | desired | marker picking if exposed |
| Primitive | required | desired | desired | primitive picking |
| Segment | required | desired | desired | stroke/path picking |
| Path | required | desired | desired | path picking if exposed |
| Image | required | required | required | image probe |
| Mesh | required | desired | required | arcball and mesh picking |
| Sphere | required | desired | required | sphere picking |
| Volume | required | desired | required | proxy picking or probe |
| Text/labels | required when public | desired | desired | label probe/readout |
| Splat | experimental | desired | desired | splat picking only if the experimental API exposes it |
| Polygon | required | desired | desired | polygon selection if exposed |

For every visual family with a public API, agents should be able to find one complete source file
that answers: "How do I create this visual with valid data?" More advanced examples should then
answer: "How do I update it?", "How do I render it offscreen?", and "How do I inspect user
interaction or readback results?"


## Feature Examples

Required or high-priority feature examples are grouped by what the user should learn. A feature
example may use a simple point, image, path, or mesh visual as scaffolding, but the feature must be
the visible point of the example.

### Scene, Layout, And Data Flow

| ID | Source | State | Expected rendered result | Teaches and limits |
| --- | --- | --- | --- | --- |
| `feature.scene_basic` | `examples/c/features/scene_basic.c` | `planned` | One panel with one tiny visual rendered successfully. | Teaches app, scene, figure, panel, visual creation, render/run, and cleanup. Avoid every optional feature. |
| `feature.panel_single` | `examples/c/features/panel_single.c` | `planned` | One figure with a single framed panel and one simple visual. | Teaches panel ownership and viewport basics. Do not duplicate `scene_basic` beyond panel-specific calls. |
| `feature.panel_grid` | `examples/c/features/panel_grid.c` | `planned` | A small 2x2 grid where each panel has a simple distinct visual or background. | Teaches grid layout and panel addressing. Avoid linked interactions. |
| `feature.panel_multi` | `examples/c/features/panel_multi.c` | `planned` | Multiple panels with independent views, showing that each panel clips and transforms correctly. | Teaches multi-panel rendering and panel-local controllers. Avoid synchronization. |
| `feature.panel_linked` | `examples/c/features/panel_linked.c` | `planned` | Two or more panels where pan/zoom or camera state is visibly linked. | Teaches shared controller or linked state. Avoid colorbar/probe complexity. |
| `feature.update_visual_data` | `examples/c/features/update_visual_data.c` | `planned` | A visual changes position, color, or size over a few deterministic frames. | Teaches retained data replacement or update API. Avoid streaming performance claims. |
| `feature.update_partial` | `examples/c/features/update_partial.c` | `planned` | Only a highlighted subset of a larger visual changes while the rest remains stable. | Teaches partial uploads and item ranges. Avoid using it as a large-data benchmark. |
| `feature.visibility` | `examples/c/features/visibility.c` | `planned` | A small scene where one visual can be hidden and shown deterministically. | Teaches retained visual visibility state. Avoid GUI controls unless the feature is specifically GUI. |

### Fields, Scales, And Adornments

| ID | Source | State | Expected rendered result | Teaches and limits |
| --- | --- | --- | --- | --- |
| `feature.sampled_field_2d` | `examples/c/features/sampled_field_2d.c` | `planned` | A 2D scalar field rendered through a plain image or pixel visual. | Teaches sampled-field resource shape and mapping. Put colorbar in `feature.colorbar`. |
| `feature.sampled_field_3d` | `examples/c/features/sampled_field_3d.c` | `planned` | A small 3D field rendered as a volume or slice with deterministic range. | Teaches 3D sampled-field resource setup. Avoid full volume interaction UI. |
| `feature.colormap_scale` | `examples/c/features/colormap_scale.c` | `planned` | One scalar-colored visual with a perceptually uniform colormap and clear min/max effect. | Teaches scalar-to-color mapping. Do not add a colorbar unless this row is merged with `feature.colorbar`. |
| `feature.colorbar` | `examples/c/features/colorbar.c` | `candidate` | One scalar-colored visual plus a readable continuous colorbar with range labels. | Teaches colorbar attachment and scale semantics. Avoid probe callbacks. |
| `feature.legend_categorical` | `examples/c/features/legend_categorical.c` | `conditional` | A small categorical visual with a compact legend mapping colors or shapes to labels. | Include only if categorical legends are public. Avoid statistical or plotting-layer semantics. |
| `feature.axes_2d` | `examples/c/features/axes_2d.c` | `candidate` | A simple 2D scatter or path with ticks, labels, and data-space bounds visible. | Teaches axis creation, bounds, and coordinate mapping. Avoid colorbar, selection, and linked panels. |
| `feature.axis_labels` | `examples/c/features/axis_labels.c` | `planned` | Axes with explicit title or axis labels, using enough margin to prove layout. | Teaches label placement around axes. Avoid long text layout stress tests. |
| `feature.scalebar` | `examples/c/features/scalebar.c` | `candidate` | One 2D panel with a small reference visual and one retained scale bar. | Teaches scale-bar attachment, units, anchor, and domain-aware sizing. Keep composed 2D/3D measurement layouts in workflows. |
| `feature.scalebar_units` | `examples/c/features/scalebar_units.c` | `candidate` | One time-series panel whose X data units are milliseconds and whose scale bar uses a custom `ms` unit string. | Teaches practical unit labeling through `unit` and `data_to_unit` without custom formatter callbacks. Avoid multiple panels or broad domain-specific unit systems. |
| `feature.annotation_label` | `examples/c/features/annotation_readout.c` | `candidate` | A point, region, or mesh feature annotated by a short anchored label or readout. | Teaches anchored annotation placement. Avoid rich text blocks and overlay cards. |
| `feature.text_block` | `examples/c/features/text_block.c` | `planned` | A compact text block or multiline note with stable screen placement. | Teaches text layout as an adornment. Do not expose internal glyph implementation. |
| `feature.overlay_card` | `examples/c/features/overlay_card.c` | `planned` | A small screen-space overlay card with text and optional swatch/readout over a scene. | Teaches overlay placement and composition. Avoid dashboard UI scope. |

### Controllers And Interaction

| ID | Source | State | Expected rendered result | Teaches and limits |
| --- | --- | --- | --- | --- |
| `feature.controller_panzoom` | `examples/c/features/controller_panzoom.c` | `planned` | A 2D point, path, or image scene where pan and zoom visibly preserve data-space meaning. | Teaches panzoom attachment and bounds. Avoid axes unless validating bounds is impossible without them. |
| `feature.controller_arcball` | `examples/c/features/controller_arcball.c` | `planned` | A centered 3D mesh or sphere group where rotation is visually meaningful. | Teaches arcball attachment. Keep lighting/materials minimal. |
| `feature.controller_fly` | `examples/c/features/controller_fly.c` | `planned` | A sparse 3D scene or point cloud where camera translation is visible. | Teaches fly navigation. Avoid dense LiDAR showcase styling. |
| `feature.controller_turntable` | `examples/c/features/controller_turntable.c` | `planned` | A 3D object rotating around a stable up axis. | Teaches constrained turntable navigation. Do not duplicate arcball behavior. |
| `feature.pick_point` | `examples/c/features/pick_point.c` | `planned` | Sparse points with a visible selected or hovered point and printed or displayed item id. | Teaches callback, query result, and stable item index. Avoid dense performance scenes. |
| `feature.pick_marker` | `examples/c/features/pick_marker.c` | `candidate` | Distinct markers where a picked item changes outline, color, or selection state. | Teaches marker picking and item identity. Do not use as the marker visual baseline. |
| `feature.pick_hover` | `examples/c/features/pick_hover.c` | `planned` | Hover feedback follows pointer movement and clears on background miss. | Teaches latest-request-wins hover behavior and miss handling. Avoid persistent selection policy. |
| `feature.probe_image` | `examples/c/features/image_probe.c` | `candidate` | Image field with a cursor or pinned marker showing data coordinates and sampled value. | Teaches image probing and pixel-query readback. Keep colorbars and textual readout annotations in separate feature examples. |
| `feature.probe_labels` | `examples/c/features/probe_labels.c` | `planned` | Label field or labeled regions where hovering reports stable label ids and names. | Teaches label probing. Avoid segmentation editor scope. |
| `feature.selection` | `examples/c/features/selection.c` | `planned` | A small visual where selected items remain highlighted after a click or scripted selection. | Teaches selection model and visual feedback. Avoid multi-visual selection linking. |
| `feature.panel_linked_probe` | `examples/c/features/panel_linked_probe.c` | `needs-rc1-proof` | Two linked panels, one scalar image, one context/detail view, with colorbar and readout synchronized. | Teaches composed explanatory layout. This is not a minimal image, colorbar, or panel example. |

### Materials And Appearance

| ID | Source | State | Expected rendered result | Teaches and limits |
| --- | --- | --- | --- | --- |
| `feature.material_mesh` | `examples/c/features/material_mesh.c` | `planned` | One mesh rendered with a neutral material where normals and shading are clear. | Teaches mesh material parameters. Avoid texture sampling. |
| `feature.mesh_texture` | `examples/c/features/mesh_texture.c` | `needs-rc1-proof` | A textured mesh with UVs and visible texture orientation, ideally with a simple checker or planet texture. | Teaches mesh-bound texture resources and UVs. Do not turn into a terrain/planet showcase. |
| `feature.lighting` | `examples/c/features/lighting.c` | `planned` | A simple 3D object where changing light direction or intensity is visibly meaningful. | Teaches light setup. Avoid material matrix demos. |

### Animation And Media

| ID | Source | State | Expected rendered result | Teaches and limits |
| --- | --- | --- | --- | --- |
| `feature.timer_animation` | `examples/c/features/timer_animation.c` | `planned` | A simple animated visual driven by a timer or frame callback. | Teaches app callbacks and animation loop. Avoid video export. |
| `feature.video_export` | `examples/c/features/video_export.c` | `conditional` | A deterministic short animation captured as a video artifact. | Include only if video export is in the public v0.4 surface. Keep backend requirements explicit. |

The query, pick, probe, and selection examples should be treated as normal first-class examples once
the current API overhaul lands.

Current feature seeds and migration notes:

| ID | Useful existing source or page | Migration note |
| --- | --- | --- |
| `feature.axes_2d` | `examples/c/features/axes_2d.c`, `docs/gallery/features/axes.md` | Replace old docs with C-first `path_axes_2d` or scatter/axes proof. |
| `feature.panel_linked` | `examples/c/workflows/panel_linked_axes.c`, `docs/gallery/features/panel.md` | Current proof is a workflow because it combines linked panels, panzoom, and axes; extract a smaller feature-only panel-linking example later. |
| `feature.scalebar` | `examples/c/features/scalebar.c`, `examples/c/features/scalebar_units.c`, `examples/c/workflows/scalebar_measurement.c`, `examples/c/lab/scalebar_2d_3d.c` | Candidate public feature proof is intentionally minimal; `scalebar_units.c` shows a non-spatial time-unit label, richer overview/detail/3D measurement composition lives in the workflow lane, and the 2D/3D comparison stays in lab. |
| `feature.colorbar` | `examples/c/features/colorbar.c` | Candidate standalone scalar scale/colorbar proof; do not add probing or linked panels. |
| `feature.annotation_label` | `examples/c/features/annotation_readout.c` | Candidate standalone anchored text/readout proof; keep data query behavior elsewhere. |
| `feature.probe_image` | `examples/c/features/image_probe.c` | Candidate focused image probe using a sampled scalar field and pixel query; colorbar and annotation/readout are split into separate examples. |
| `feature.pick_marker` | `examples/c/features/pick_marker.c` | Candidate picking/selection proof; do not use as the marker visual baseline. |
| `feature.mesh_texture` | `examples/c/showcases/textured_planet.c`, `examples/c/legacy/visuals/textured_mesh.c` | Use as retained textured-mesh proof; keep planet/terrain polish in showcase. |
| `feature.controller_arcball` | `examples/c/scientific/protein.c`, `examples/c/showcases/textured_planet.c` | Extract a minimal controller proof from composed examples. |
| `feature.controller_fly` | `examples/c/legacy/showcase/lidar.c` | Salvage only after dense point/EDL showcase scope is settled. |
| `feature.timer_animation` | `docs/gallery/features/animation.md`, `docs/gallery/features/timer.md` | Replace old pages with one deterministic callback/animation example. |
| `feature.video_export` | `docs/gallery/features/video.md` | Fold into experimental animation/video export if video remains public. |
| `feature.lighting` | `docs/gallery/features/light.md`, `docs/gallery/features/mesh_light.md` | Salvage snippets; prefer a smaller mesh/sphere lighting proof. |
| `feature.visibility` | `docs/gallery/features/hide.md`, `docs/gallery/features/fixed.md` | Keep as API/reference material unless a public visibility example is useful. |
| `feature.annotation_label` | `examples/c/legacy/techniques/overlay_card.c`, `examples/c/legacy/techniques/rich_text_block.c` | Split annotation, text block, and overlay card into separate feature rows. |
| `runtime.capture_png` | `examples/c/lab/record_dvzr.c`, `examples/c/lab/replay_dvzr.c` | Useful runtime evidence, but PNG capture should be its own small public example. |
| `portability.webgpu_subset` | `examples/c/legacy/tools/export_point_wgsl.c`, `examples/c/legacy/tools/export_image_wgsl.c`, `examples/c/legacy/tools/export_primitive_wgsl.c` | Use as fixture/export evidence; public WebGPU page needs explicit experimental status. |


## Runtime Examples

Runtime examples document how a program is hosted or executed:

| ID | Source | Expected rendered result or artifact | Teaches and limits |
| --- | --- | --- | --- |
| `runtime.window_glfw` | `examples/c/runtime/window_glfw.c` | A bounded native window with one simple scene. | Teaches windowed app hosting. Avoid scene feature complexity. |
| `runtime.offscreen` | `examples/c/runtime/offscreen.c` | A scene rendered without a visible window. | Teaches offscreen context and render lifecycle. |
| `runtime.capture_png` | `examples/c/runtime/capture_png.c` | A deterministic PNG capture with nonblank image validation. | Teaches capture path and artifact writing. |
| `runtime.frame_callback` | `examples/c/runtime/frame_callback.c` | A simple per-frame state change or counter. | Teaches frame callbacks. Avoid animation polish. |
| `runtime.continuous` | `examples/c/runtime/continuous.c` | A scene rendered continuously or on demand with clear scheduling behavior. | Teaches immediate versus continuous rendering. |
| `runtime.qt_hosted` | existing Qt example path, linked when supported | Hosted rendering in a Qt surface. | Only public when Qt integration is supported and documented. |


## Technique Examples

Technique examples may compose a small number of visuals and runtime features to demonstrate a
rendering technique:

| ID | Source | Expected rendered result | Teaches and limits |
| --- | --- | --- | --- |
| `technique.transparency` | `examples/c/techniques/transparency.c` | Overlapping translucent objects with ordering behavior visible. | Teaches basic transparency constraints. |
| `technique.wboit` | `examples/c/techniques/wboit.c` | Side-by-side or toggled weighted blended OIT effect. | Teaches WBOIT, backend requirements, and limitations. |
| `technique.msaa` | `examples/c/techniques/msaa.c` | Geometry with clear edge aliasing improvement. | Teaches MSAA configuration. |
| `technique.depth_testing` | `examples/c/techniques/depth_testing.c` | Overlapping 3D objects that make depth testing or ordering behavior obvious. | Teaches depth-buffer behavior. Keep depth cueing, transparency, and WBOIT separate. |
| `technique.edl` | `examples/c/techniques/edl.c` | Dense point or pixel cloud with depth enhancement visible. | Teaches EDL as a rendering technique, not point visual basics. |
| `technique.ssao` | `examples/c/techniques/ssao.c` | 3D mesh or sphere scene with ambient occlusion visible. | Teaches SSAO configuration. |
| `technique.depth_cue` | `examples/c/techniques/depth_cue.c` | 3D objects fading or scaling with depth. | Teaches depth cueing. |
| `technique.bounds_overlay` | `examples/c/techniques/bounds_overlay.c` | Visual bounds or debug overlays drawn around known objects. | Keep diagnostic status explicit; do not make this a normal visual example. |


## DRP2 And Portability Examples

| Area | Example |
| --- | --- |
| Raw triangle | existing raw triangle DRP2 example |
| Record DVZR | existing `record_dvzr` tool/example |
| Replay DVZR | existing `replay_dvzr` tool/example |
| WebGPU point | WebGPU fixture or runnable browser example |
| WebGPU image | WebGPU fixture or runnable browser example |
| WebGPU primitive | WebGPU fixture or runnable browser example |
| WebGPU mesh | when included in the supported experimental subset |


## Showcase Examples

Showcases are allowed to be attractive, composed, and domain-flavored. They should not be minimal
or exhaustive.

Candidate showcases:

| Showcase | Demonstrates |
| --- | --- |
| LiDAR | large point cloud, colormap, controller, performance |
| Brain image and labels | image, labels, probing, colorbar |
| Molecule or protein | mesh/sphere, material, lighting |
| Volume slice | volume, sampled field, slicing, probe |
| Multi-panel dashboard | panels, linked views, axes, colorbars |
| Mesh technique demo | mesh plus SSAO, EDL, or MSAA |
| Annotation/readout demo | picking, overlay cards, label annotations |

Showcase pages should declare `agent_copy_safe: false` unless they are intentionally written as a
small public starting point. They may link to multiple minimal examples for the individual
features they compose.


## Legacy Gallery Handling

The current `docs/gallery/**` pages may contain useful screenshots, data choices, or snippets, but
they should be classified before v0.4 publication:

| Classification | Meaning |
| --- | --- |
| `replace` | The old page teaches the wrong API or concept; write a new v0.4 page from the matrix. |
| `salvage-idea` | Keep the visual idea, data shape, or screenshot composition, but rewrite source and docs. |
| `candidate` | The page or nearby source already matches the v0.4 contract after minor cleanup. |
| `defer` | Useful later, but not required for v0.4 one-visual or one-feature coverage. |
| `delete` | Remove from public v0.4 docs; keep history in git. |

Default classifications:

1. v0.3 Python-first visual pages are `replace` unless they already describe the v0.4 C-first
   scene/app path.
2. `basic` and `wiggle` visual pages are `delete` or `defer` for v0.4 minimal coverage. The old
   `glyph` page may be salvaged only through the public `visual.text` row.
3. Feature pages that bundle several unrelated topics are `salvage-idea`; split them into one
   feature row each.
4. Showcase pages may be `salvage-idea` even when their source is obsolete, because dataset and
   composition choices can remain valuable.

Current high-level docs actions:

| Current docs area | v0.4 action |
| --- | --- |
| `docs/gallery/visuals/basic.md` | Replace with `primitive`; remove `basic` as a public visual family. |
| `docs/gallery/visuals/wiggle.md` | Delete or defer; treat as a future `path` variant, not a top-level family. |
| `docs/gallery/visuals/glyph.md` | Salvage only as public text documentation. |
| `docs/gallery/visuals/{point,pixel,path,segment,image,mesh,sphere,volume}.md` | Salvage idea and screenshots only; rewrite as C-first v0.4 examples. |
| `docs/gallery/features/{axes,panel,arcball,fly,offscreen}.md` | Replace with targeted C-first feature or runtime pages. |
| `docs/gallery/features/{colorbar,colormaps}.md` | Salvage into scalar field, colormap, colorbar, and image-probe feature rows. |
| `docs/gallery/features/polygon.md` | Rebuild as `docs/gallery/visuals/polygon.md`; polygon is a v0.4 visual-family example. |
| `docs/gallery/features/{animation,timer,video}.md` | Replace with one deterministic animation example plus optional video-export page. |
| `docs/gallery/features/{keyboard,mouse,camera,orbit,timestamps,gui_panel}.md` | Defer unless the corresponding public v0.4 surface is explicitly kept. |
| `docs/gallery/features/{fixed,hide,stop}.md` | Delete from public gallery or move to reference/API notes. |


## Scientific Dataset Showcases

Scientific showcases should prefer real public datasets when the data makes a better example than
synthetic content. Keep showcase selection and scientist outreach policy in
[`../release/GALLERY_OUTREACH.md`](../release/GALLERY_OUTREACH.md). Keep data layout, provenance,
promotion, and submodule policy in
[`../data/V0_4_DATA_REPOSITORY.md`](../data/V0_4_DATA_REPOSITORY.md).


## Example Metadata

Every documented example should eventually have machine-readable metadata. The format may live in a
central manifest or near each example, but it should contain these fields:

```yaml
id: visual.point
title: Point visual
kind: visual
source: examples/c/visuals/point.c
status: supported
agent_copy_safe: true
role: minimal
features:
  - scene
  - point
backends:
  - native
screenshot: screenshots/visuals/point.png
tests:
  - just test scene
docs:
  reference: reference/visual-families/index.md#point
  how_to: how-to/add-a-visual.md
notes:
  - Uses the scene/app ownership pattern.
expected_scene: Deterministic 2D point grid with visible per-point color and size variation.
teaches:
  - retained point positions
  - per-item color
  - per-item size
allowed_extras:
  - panzoom for inspection
must_not_show:
  - axes
  - picking
  - animation
```

This metadata should support:

1. generated example galleries;
2. feature coverage reports;
3. release checklists;
4. stable LLM retrieval;
5. checks for missing examples, screenshots, or reference pages.
6. automatic filtering of copy-safe examples versus sketches or showcases.


## Minimal Example Rules

1. Prefer one source file per example.
2. Keep setup explicit and easy to copy.
3. Use the fewest visuals necessary.
4. Avoid hidden global state.
5. Include a short top-of-file comment describing the demonstrated visual or feature.
6. Keep validation commands in the corresponding docs or metadata, not as stale comments in code.
7. If an example requires optional runtime support, label the backend and platform constraints.
8. Avoid unrelated helper abstractions that hide object creation, data binding, or cleanup.
9. Use current public headers and avoid old v0.3 Pythonic names even in comments.
10. Keep destroy and cleanup calls visible unless the example intentionally demonstrates borrowed
    runtime ownership.
