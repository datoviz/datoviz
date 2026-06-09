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
3. One public semantic composite gets one minimal C example.
4. Rendering techniques count as features unless they become public visual families.
5. A minimal example should avoid unrelated visual polish.
6. A showcase may compose many visuals, features, composites, techniques, and workflows, but it does
   not satisfy the minimal-example requirement.
7. Every example should be linked from the relevant reference and how-to pages.
8. Examples should have stable identifiers that can be used by docs, tests, release notes, and LLM
   retrieval.
9. Copy-safe examples should use the preferred scene/app ownership pattern and declare their
   validation command.
10. If an example is only an API sketch, pressure test, or showcase, mark it so agents do not copy it
   as a minimal starting point.
11. How-to pages should not be created one-to-one for every feature. Add a how-to page when a user
   task needs workflow guidance, tradeoffs, ownership notes, or adaptation advice.
12. Visual documentation should include authored judgment such as "use when", "avoid when", and
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
| `v0.4-required-planned` | Must be implemented as a short public C example for v0.4 release coverage. |
| `candidate` | A source file exists or legacy source can be salvaged, but docs/capture may need rewrite. |
| `ready-now` | Current source can be polished without waiting for release blockers. |
| `needs-rc1-proof` | Feature exists, but needs a runnable example, capture, fixture, or validation pass. |
| `experimental` | In the v0.4 public experimental subset; docs and metadata must label it honestly. |
| `conditional` | Include only if the visual or feature remains public for v0.4. |
| `deferred` | Do not force into v0.4 minimal coverage. |
| `external/GSP` | Belongs primarily outside Datoviz C examples. |


Recommended gallery capabilities:

| Capability | Meaning |
| --- | --- |
| `static-media` | The example has a stable PNG and/or video artifact for docs and release notes. |
| `wasm-supported` | The example is expected to compile and run through the portable WASM/WebGPU path. |
| `gallery-live` | The example is curated for public live embedding in the website gallery. |

Treat `wasm-supported` as a validation target, not a publishing promise. Most portable examples may
eventually build to WASM, while `gallery-live` should remain a smaller curated subset backed by
fallback static media and honest unsupported-feature diagnostics.


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
  composites/
  showcases/
  lab/
  legacy/
```

The public taxonomy should stay this small:

| Category | Unit | Notes |
| --- | --- | --- |
| `visuals` | one public visual family | Mechanical proof for the data shape and smallest useful styling of one visual. |
| `features` | one isolated feature or technique | Mechanical proof for one capability, using the simplest visual scaffolding needed. |
| `composites` | one semantic scene object | Mechanical proof for objects that lower to coordinated visual roles. |
| `showcases` | one composed goal | Workflows, scientific examples, shiny demos, real-data stories, and multi-feature scenes. |

Do not add public source folders for `workflow`, `scientific`, `technique`, or domain labels unless a
future build-system constraint requires it. Prefer metadata tags such as `workflow`, `real-data`,
`simulated`, `scientific`, `interactive`, `offscreen`, `compute`, `geo`, or `molecular`.

WebGPU examples and fixtures may keep their existing browser-oriented layout under `examples/webgpu/`
while being indexed by the same documentation manifest.


## Visual Examples

Required dedicated visual examples are below. Each row describes the intended v0.4 public example,
not the old gallery page that may currently occupy a similar slot.

| ID | Source | State | Expected rendered result | Teaches and limits |
| --- | --- | --- | --- | --- |
| `visual.point` | `examples/c/visuals/point.c` | `ready-now` | A deterministic compact cloud of points with visible per-point color and size variation. | Teaches retained point positions, colors, and sizes. Avoid axes, picking, animation, and complex colormaps. |
| `visual.pixel` | `examples/c/visuals/pixel.c` | `ready-now` | A dense raster-like grid of square pixels, with clear value variation at thumbnail size. | Teaches pixel positions, sizes, and color/value mapping. Avoid image texture APIs and probe readouts. |
| `visual.marker` | `examples/c/visuals/marker.c` | `ready-now` | A small arranged set of marker shapes large enough to inspect, with shape, fill/stroke color, and size variation. | Teaches marker shape and styling attributes. Put picking in `feature.pick_marker`, not here. |
| `visual.primitive` | `examples/c/visuals/primitive.c` | `ready-now` | A few simple primitives arranged so topology differences are obvious, with distinct colors. | Teaches topology-parametric primitive data. Avoid shape builders, polygons, and mesh indexing. |
| `visual.segment` | `examples/c/visuals/segment.c` | `ready-now` | Independent line segments with varying color and width, visibly not connected into a continuous path. | Teaches endpoint-pair data and per-segment styling. Avoid path joins, axes, and vector arrowheads. |
| `visual.path` | `examples/c/visuals/path.c` | `ready-now` | Continuous deterministic signals with clear stroke styling. | Teaches ordered path data, path breaks, width, and color. Avoid axes unless implementing `feature.axes_2d`. |
| `visual.vector` | `examples/c/visuals/vector.c` | `ready-now` | A small quiver or flow field where direction, length, and optional arrowheads are readable. | Teaches vector anchors, vector values, scale, and color. Avoid wind-field showcase polish and animation. |
| `visual.image` | `examples/c/visuals/image.c` | `ready-now` | A scalar 2D field filling most of the panel, with enough structure to see orientation and sampling. | Teaches image or 2D sampled-field upload. Put colorbar and probing in feature examples. |
| `visual.mesh` | `examples/c/visuals/mesh.c` | `ready-now` | A simple indexed 3D surface with readable shape, normals or flat colors, and a stable camera pose. | Teaches vertices, indices, mesh creation, and basic 3D viewing. Put texture in `feature.mesh_texture`. |
| `visual.sphere` | `examples/c/visuals/sphere.c` | `ready-now` | A small 3D cluster of impostor spheres with depth, radius, and color variation visible. | Teaches sphere centers, radii, colors, and depth behavior. Avoid molecule-showcase semantics. |
| `visual.volume` | `examples/c/visuals/volume.c` | `ready-now` | A compact 3D volume rendering or slice stack with clear internal structure and deterministic transfer defaults. | Teaches 3D sampled-field or volume visual setup. Avoid full medical-viewer controls. |
| `visual.text` | `examples/c/visuals/text.c` | `ready-now` | A few short strings placed in data and/or screen space with legible anchors, color, and size. | Teaches semantic text creation if text is public. Do not expose internal glyph visual details. |
| `visual.glyph` | `examples/c/visuals/glyph.c` | `experimental` | A few atlas-backed quads with explicit bounds, UVs, colors, and angles. | Raw glyph coverage only; ordinary users should prefer `visual.text`. |
| `visual.labels` | `examples/c/visuals/labels.c` | `ready-now` | An integer label field or labeled regions with distinct categorical colors and stable ids. | Teaches label-field upload and categorical styling if labels are public. Put probe/readout in `feature.probe_labels`. |
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
| `visual.point` | `examples/c/visuals/point.c`, `docs/gallery/visuals/point.md` | Current C example is the source of truth; salvage only screenshot/composition ideas from old docs. |
| `visual.pixel` | `examples/c/visuals/pixel.c`, `docs/gallery/visuals/pixel.md` | Current C example is the source of truth; keep image upload and probing elsewhere. |
| `visual.marker` | `examples/c/visuals/marker.c`, `examples/c/features/pick_marker.c` | Current marker baseline is separate from picking. Use `pick_marker.c` only as feature evidence. |
| `visual.primitive` | `examples/c/visuals/primitive.c`, `docs/gallery/visuals/basic.md` | Replace the old `basic` page with `primitive`; there is no v0.4 `basic` visual family. |
| `visual.segment` | `examples/c/visuals/segment.c`, `docs/gallery/visuals/segment.md` | Current C example is the source of truth; keep paths and vectors separate. |
| `visual.path` | `examples/c/visuals/path.c`, `docs/gallery/visuals/path.md` | Current C example is the source of truth; axes belong in `feature.axes_2d`. |
| `visual.vector` | `examples/c/visuals/vector.c`, `examples/c/showcases/wind_field.c` | Current C examples cover straight/curved vectors and a composed wind-field proof. |
| `visual.image` | `examples/c/visuals/image.c`, `examples/c/features/image_probe.c` | Current image baseline is separate from probing and colorbar/readout behavior. |
| `visual.mesh` | `examples/c/visuals/mesh.c`, `docs/gallery/visuals/mesh.md` | Current C example is the source of truth; textured mesh is a separate feature/showcase proof. |
| `visual.sphere` | `examples/c/visuals/sphere.c`, `examples/c/showcases/protein.c` | Current sphere baseline is separate from molecule/protein scientific material. |
| `visual.volume` | `examples/c/visuals/volume.c`, `examples/c/showcases/brain_volume.c` | Current volume baseline is separate from full brain/medical composition. |
| `visual.text` | `examples/c/visuals/text.c`, `examples/c/lab/text_msdf_diagnostics.c` | Current text baseline uses the public text API; do not document raw glyph internals. |
| `visual.labels` | `examples/c/visuals/labels.c`, `examples/c/features/probe_labels.c` | Current labels baseline is separate from label probing. |
| `visual.splat` | `examples/c/visuals/splat.c`, `examples/c/legacy/showcase/gothic_splat.c` | Experimental v0.4 visual; publish with explicit experimental labeling and no full asset-pipeline promise. |
| `composite.polygon` | `examples/c/composites/polygon.c`, `docs/gallery/composites/polygon.md` | Polygon is in v0.4 release scope as a semantic composite; rebuild the old feature page as a C-first composite page. |


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

Required or high-priority feature examples are grouped by what the user should learn. For v0.4
release, every public feature should have one short C example under `examples/c/features/`. Rows
marked `ready-now` have current C sources and still need normal capture/docs publication before the
gallery is final. A feature example may use a simple point, image, path, or mesh visual as
scaffolding, but the feature must be the visible point of the example.

### Scene, Layout, And Data Flow

| ID | Source | State | Expected rendered result | Teaches and limits |
| --- | --- | --- | --- | --- |
| `feature.basic_scene` | `examples/c/features/basic_scene.c` | `ready-now` | One panel with one tiny visual rendered successfully. | Teaches app, scene, figure, panel, visual creation, render/run, and cleanup. Avoid every optional feature. |
| `feature.panel_single` | `examples/c/features/panel_single.c` | `ready-now` | One figure with a single framed panel and one simple visual. | Teaches panel ownership and viewport basics. Do not duplicate `basic_scene` beyond panel-specific calls. |
| `feature.panel_grid` | `examples/c/features/panel_grid.c` | `ready-now` | A small 2x2 grid where each panel has a simple distinct visual or background. | Teaches grid layout and panel addressing. Avoid linked interactions. |
| `feature.panel_multi` | `examples/c/features/panel_multi.c` | `ready-now` | Multiple panels with independent views, showing that each panel clips and transforms correctly. | Teaches multi-panel rendering and panel-local controllers. Avoid synchronization. |
| `feature.panel_linked` | `examples/c/features/panel_linked.c` | `ready-now` | Two or more panels where pan/zoom or camera state is visibly linked. | Teaches shared controller or linked state. Avoid colorbar/probe complexity. |
| `feature.panel_background` | `examples/c/features/panel_background.c` | `ready-now` | One fixed panel background with a simple foreground visual. | Teaches panel-level background styling. Keep overlay/card placement in `feature.overlay_card`. |
| `feature.update_visual_data` | `examples/c/features/update_visual_data.c` | `ready-now` | A visual changes position, color, or size over a few deterministic frames. | Teaches retained data replacement or update API. Avoid streaming performance claims. |
| `feature.update_partial` | `examples/c/features/update_partial.c` | `ready-now` | Only a highlighted subset of a larger visual changes while the rest remains stable. | Teaches partial uploads and item ranges. Avoid using it as a large-data benchmark. |
| `feature.visibility` | `examples/c/features/visibility.c` | `ready-now` | A small scene where one visual can be hidden and shown deterministically. | Teaches retained visual visibility state. Avoid GUI controls unless the feature is specifically GUI. |

### Fields, Scales, And Adornments

| ID | Source | State | Expected rendered result | Teaches and limits |
| --- | --- | --- | --- | --- |
| `feature.sampled_field_2d` | `examples/c/features/sampled_field_2d.c` | `ready-now` | A 2D scalar field rendered through a plain image or pixel visual. | Teaches sampled-field resource shape and mapping. Put colorbar in `feature.colorbar`. |
| `feature.sampled_field_3d` | `examples/c/features/sampled_field_3d.c` | `ready-now` | A small 3D field rendered as a volume or slice with deterministic range. | Teaches 3D sampled-field resource setup. Avoid full volume interaction UI. |
| `feature.colormap_scale` | `examples/c/features/colormap_scale.c` | `ready-now` | One scalar-colored visual with a perceptually uniform colormap and clear min/max effect. | Teaches scalar-to-color mapping. Do not add a colorbar unless this row is merged with `feature.colorbar`. |
| `feature.colorbar` | `examples/c/features/colorbar.c` | `ready-now` | One scalar-colored visual plus a readable continuous colorbar with range labels. | Teaches colorbar attachment and scale semantics. Avoid probe callbacks. |
| `feature.legend_categorical` | `examples/c/features/legend_categorical.c` | `experimental` | A small categorical visual with a compact legend mapping colors or shapes to labels. | Include only if categorical legends are public. Avoid statistical or plotting-layer semantics. |
| `feature.axes_2d` | `examples/c/features/axes_2d.c` | `ready-now` | A simple 2D scatter or path with ticks, labels, and data-space bounds visible. | Teaches axis creation, bounds, and coordinate mapping. Avoid colorbar, selection, and linked panels. |
| `feature.axis_labels` | `examples/c/features/axis_labels.c` | `ready-now` | Axes with explicit title or axis labels, using enough margin to prove layout. | Teaches label placement around axes. Avoid long text layout stress tests. |
| `feature.scalebar` | `examples/c/features/scalebar.c` | `ready-now` | One 2D panel with a small reference visual and one retained scale bar. | Teaches scale-bar attachment, units, anchor, and domain-aware sizing. Keep composed 2D/3D measurement layouts in workflows. |
| `feature.scalebar_units` | `examples/c/features/scalebar_units.c` | `ready-now` | One time-series panel whose X data units are milliseconds and whose scale bar uses a custom `ms` unit string. | Teaches practical unit labeling through `unit` and `data_to_unit` without custom formatter callbacks. Avoid multiple panels or broad domain-specific unit systems. |
| `feature.annotation_label` | `examples/c/features/annotation_readout.c` | `ready-now` | A point, region, or mesh feature annotated by a short anchored label or readout. | Teaches anchored annotation placement. Avoid rich text blocks and overlay cards. |
| `feature.text_block` | `examples/c/features/text_block.c` | `ready-now` | A compact text block or multiline note with stable screen placement. | Teaches text layout as an adornment. Do not expose internal glyph implementation. |
| `feature.overlay_card` | `examples/c/features/overlay_card.c` | `ready-now` | A small screen-space overlay card with text and optional swatch/readout over a scene. | Teaches overlay placement and composition. Avoid dashboard UI scope. |

### Controllers And Interaction

| ID | Source | State | Expected rendered result | Teaches and limits |
| --- | --- | --- | --- | --- |
| `feature.controller_panzoom` | `examples/c/features/panzoom.c` | `ready-now` | A 2D point, path, or image scene where pan and zoom visibly preserve data-space meaning. | Teaches panzoom attachment and bounds. Avoid axes unless validating bounds is impossible without them. |
| `feature.controller_arcball` | `examples/c/features/controller_arcball.c` | `ready-now` | A centered 3D mesh or sphere group where rotation is visually meaningful. | Teaches arcball attachment. Keep lighting/materials minimal. |
| `feature.controller_fly` | `examples/c/features/controller_fly.c` | `ready-now` | A sparse 3D scene or point cloud where camera translation is visible. | Teaches fly navigation. Avoid dense LiDAR showcase styling. |
| `feature.controller_turntable` | `examples/c/features/controller_turntable.c` | `ready-now` | A 3D object rotating around a stable up axis. | Teaches constrained turntable navigation. Do not duplicate arcball behavior. |
| `feature.pick_point` | `examples/c/features/pick_point.c` | `ready-now` | Sparse points with a visible selected or hovered point and printed or displayed item id. | Teaches callback, query result, and stable item index. Avoid dense performance scenes. |
| `feature.pick_marker` | `examples/c/features/pick_marker.c` | `ready-now` | Distinct markers where a picked item changes outline, color, or selection state. | Teaches marker picking and item identity. Do not use as the marker visual baseline. |
| `feature.pick_hover` | `examples/c/features/pick_hover.c` | `ready-now` | Hover feedback follows pointer movement and clears on background miss. | Teaches latest-request-wins hover behavior and miss handling. Avoid persistent selection policy. |
| `feature.probe_image` | `examples/c/features/image_probe.c` | `ready-now` | Image field with a cursor or pinned marker showing data coordinates and sampled value. | Teaches image probing and pixel-query readback. Keep colorbars and textual readout annotations in separate feature examples. |
| `feature.probe_labels` | `examples/c/features/probe_labels.c` | `ready-now` | Label field or labeled regions where hovering reports stable label ids and names. | Teaches label probing. Avoid segmentation editor scope. |
| `feature.selection` | `examples/c/features/selection.c` | `ready-now` | A small visual where selected items remain highlighted after a click or scripted selection. | Teaches selection model and visual feedback. Avoid multi-visual selection linking. |
| `workflow.linked_probe_colorbar` | `examples/c/showcases/linked_probe_colorbar.c` | `ready-now` | Two linked panels, one scalar image, one context/detail view, with colorbar and readout synchronized. | Teaches composed explanatory layout. This is not a minimal image, colorbar, or panel example. |

### Materials And Appearance

| ID | Source | State | Expected rendered result | Teaches and limits |
| --- | --- | --- | --- | --- |
| `feature.material_mesh` | `examples/c/features/material_mesh.c` | `ready-now` | One mesh rendered with a neutral material where normals and shading are clear. | Teaches mesh material parameters. Avoid texture sampling. |
| `feature.mesh_texture` | `examples/c/features/mesh_texture.c` | `ready-now` | A textured mesh with UVs and visible texture orientation, ideally with a simple checker or planet texture. | Teaches mesh-bound texture resources and UVs. Do not turn into a terrain/planet showcase. |
| `feature.lighting` | `examples/c/features/lighting.c` | `ready-now` | A simple 3D object where changing light direction or intensity is visibly meaningful. | Teaches light setup. Avoid material matrix demos. |
| `feature.depth_test` | `examples/c/features/technique_depth_test.c` | `ready-now` | Side-by-side overlapping marks show depth testing enabled and disabled. | Teaches `dvz_visual_set_depth_test()` only. Keep depth cueing and occlusion as separate techniques. |
| `feature.alpha_blending` | `examples/c/features/alpha_blending.c` | `ready-now` | Overlapping translucent primitives blend source-over against the panel background. | Teaches per-vertex alpha with `DVZ_ALPHA_BLENDED`. Keep WBOIT and depth peeling separate. |

### Animation And Media

| ID | Source | State | Expected rendered result | Teaches and limits |
| --- | --- | --- | --- | --- |
| `feature.timer_animation` | `examples/c/features/timer_animation.c` | `ready-now` | A simple animated visual driven by a timer or frame callback. | Teaches app callbacks and animation loop. Avoid video export. |
| `feature.video_export` | `examples/c/features/video_export.c` | `experimental` | A deterministic short animation captured as a video artifact. | Include only if video export is in the public v0.4 surface. Keep backend requirements explicit. |

The query, pick, probe, and selection examples should be treated as normal first-class examples once
the current API overhaul lands.

Current feature seeds and migration notes:

| ID | Useful existing source or page | Migration note |
| --- | --- | --- |
| `feature.axes_2d` | `examples/c/features/axes_2d.c`, `docs/gallery/features/axes.md` | Replace old docs with C-first `path_axes_2d` or scatter/axes proof. |
| `feature.panel_linked` | `examples/c/features/panel_linked.c`, `examples/c/showcases/panel_linked_axes.c` | Minimal linked-panel proof is separate from the axes workflow. |
| `feature.scalebar` | `examples/c/features/scalebar.c`, `examples/c/features/scalebar_units.c`, `examples/c/showcases/scalebar_measurement.c` | Minimal feature proofs stay separate from the richer measurement workflow; the older 2D/3D comparison stays in lab. |
| `feature.colorbar` | `examples/c/features/colorbar.c` | Standalone scalar scale/colorbar proof; do not add probing or linked panels. |
| `feature.annotation_label` | `examples/c/features/annotation_readout.c` | Standalone anchored text/readout proof; keep data query behavior elsewhere. |
| `feature.probe_image` | `examples/c/features/image_probe.c` | Focused image probe using a sampled scalar field and pixel query; colorbar and annotation/readout are split into separate examples. |
| `feature.pick_marker` | `examples/c/features/pick_marker.c` | Picking/selection proof; do not use as the marker visual baseline. |
| `feature.mesh_texture` | `examples/c/features/mesh_texture.c`, `examples/c/showcases/textured_planet.c` | Minimal textured-mesh proof is separate from the planet showcase. |
| `feature.controller_arcball` | `examples/c/features/controller_arcball.c`, `examples/c/showcases/protein.c` | Minimal controller proof is separate from composed scientific/showcase examples. |
| `feature.controller_fly` | `examples/c/features/controller_fly.c`, `examples/c/showcases/point_cloud.c` | Minimal fly-controller proof is separate from the dense point-cloud showcase. |
| `feature.timer_animation` | `examples/c/features/timer_animation.c` | Current C example replaces old animation/timer gallery pages. |
| `feature.video_export` | `examples/c/features/video_export.c` | Experimental scenario-runner video proof replaces the old video gallery page. |
| `feature.lighting` | `examples/c/features/lighting.c` | Current C example replaces old light/mesh-light snippets for v0.4 docs. |
| `feature.visibility` | `examples/c/features/visibility.c` | Current C example replaces old hide/fixed gallery pages for v0.4 docs. |
| `feature.overlay_card` | `examples/c/features/overlay_card.c` | Current C example is separate from annotation and text-block proofs. |
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


## Technique Feature Examples

Technique examples are feature examples. They may compose a small number of visuals and runtime
features to demonstrate one rendering technique, but they should live under `examples/c/features/`
when they become public:

| ID | Source | Expected rendered result | Teaches and limits |
| --- | --- | --- | --- |
| `technique.transparency` | `examples/c/features/transparency.c` | Overlapping translucent objects with ordering behavior visible. | Teaches basic transparency constraints. |
| `technique.wboit` | `examples/c/features/wboit.c` | Side-by-side or toggled weighted blended OIT effect. | Teaches WBOIT, backend requirements, and limitations. |
| `technique.msaa` | `examples/c/features/technique_msaa.c` | Geometry with clear edge aliasing improvement. | Teaches MSAA configuration. |
| `technique.depth_testing` | `examples/c/features/technique_depth_testing.c` | Overlapping 3D objects that make depth testing or ordering behavior obvious. | Teaches depth-buffer behavior. Keep depth cueing, transparency, and WBOIT separate. |
| `technique.edl` | `examples/c/features/technique_edl.c` | Dense point or pixel cloud with depth enhancement visible. | Teaches EDL as a rendering technique, not point visual basics. |
| `technique.ssao` | `examples/c/features/technique_ssao.c` | 3D mesh or sphere scene with ambient occlusion visible. | Teaches SSAO configuration. |
| `technique.depth_cue` | `examples/c/features/technique_depth_cue.c` | 3D objects fading or scaling with depth. | Teaches depth cueing. |
| `technique.bounds_overlay` | `examples/c/features/bounds_overlay.c` | Visual bounds or debug overlays drawn around known objects. | Keep diagnostic status explicit; do not make this a normal visual example. |


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

Current showcase and scientific gallery-facing examples:

| Showcase | Demonstrates |
| --- | --- |
| `examples/c/showcases/point_cloud.c` | large RGB point cloud, fly controller, EDL, performance |
| `examples/c/showcases/protein.c` | real PDB atom spheres, arcball, materials, postprocess diagnostics |
| `examples/c/showcases/brain_volume.c` | Allen/IBL RGBA volume, occluded slice, arcball |
| `examples/c/showcases/wind_field.c` | scalar field, retained vectors, streamlines, animation |
| `examples/c/showcases/textured_planet.c` | textured mesh, sampled textures, lighting, arcball, video capture |
| `examples/c/showcases/gpu_particle_smoke.c` | experimental scene compute feeding point rendering |
| `examples/c/showcases/choropleth.c` | real Census polygon-set choropleth, scalar color scale, panzoom |

Additional composed gallery candidates such as brain image/labels, multi-panel dashboards, mesh
technique demos, and annotation/readout demos remain useful, but they are not missing required C
showcase sources for the current v0.4 set.

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
| `docs/gallery/features/polygon.md` | Rebuild as a C-first composite page; polygon is a v0.4 semantic composite, not a visual family. |
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
