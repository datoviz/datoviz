# Datoviz And vispy2 Example Suite Organization

> **Status:** planning
> **Scope:** Datoviz v0.4 C examples, Python binding examples, vispy2 GSP/plot examples, fixtures,
> showcases, lab material, and temporary legacy examples
> **Goal:** keep examples discoverable by purpose and API layer while sharing scenario IDs

This document defines repository ownership and layout. Release staging lives in
[PLANNING.md](PLANNING.md); one-feature fixture ideas live in
[FIXTURES.md](FIXTURES.md). Shared worked-example policy lives in
[POLICIES.md](POLICIES.md).


## Goals

- Split examples by purpose first, then by API layer.
- Keep C examples as canonical native engine/API examples.
- Keep Python binding examples in this repository as thin Datoviz binding examples.
- Keep GSP and plot examples in vispy2, where Python-native workflows belong.
- Separate public examples from fixtures, tests, lab workbenches, and temporary legacy material.
- Share stable scenario IDs across repositories.


## Repository Ownership

- Datoviz owns C public API examples, Python binding examples, engine fixtures, native
  showcases, and generated DRP2/DVZR/WebGPU fixtures. Regression coverage belongs in tests;
  stress, diagnostics, and prototypes belong in `examples/c/lab/` until promoted. It should not
  become the main high-level Python gallery once GSP/plot are active.
- vispy2 owns GSP object-oriented scene examples, plot-interface examples, Python showcases,
  notebooks, napari/Qt/dashboard integrations, and NumPy/pandas/xarray/SciPy workflows. It should
  not own native engine conformance examples or low-level runtime fixtures.

Python binding examples in Datoviz should stay close to the Datoviz API. If an example needs rich Python object
ownership, plotting conveniences, notebooks, or ecosystem data loading, it belongs in vispy2.


## API Layers

- C, in `examples/c/`: canonical public engine/scene/app examples, gallery showcases, flat lab
  material, and a temporary legacy archive.
- Python binding, in `examples/python/`: thin binding smoke/parity examples that load the library,
  create scenes, upload data, render offscreen/windowed, pick/probe/update/capture.
- GSP, in vispy2: object-oriented scene programming, retained visuals, callbacks, selections,
  linked scene state, and Pythonic animation/update patterns.
- Plot, in vispy2: task-first scatter/line/image/visuals_volume/surface/mesh plots, linked views,
  dashboards, scales, annotations, labels, and selections.


## Example Lanes

- Visuals: one active visual family at a time, such as point, pixel, marker, primitive, segment,
  path, mesh, image, volume slice/rendering, sphere impostor, and text/annotation when active.
- Features: reusable scene/app capabilities and rendering techniques, such as partial update,
  mutability hints, visibility, depth, controllers, sampled fields, colormaps, colorbar, picking,
  probing, selection, pinned readout, video export, WBOIT, EDL, SSAO, and DVZR.
- Composites: semantic scene objects such as polygon sets and graphs that lower to coordinated
  visual roles while preserving identity, topology, and styling.
- Showcases: composed user goals and polished domain demos for workflows, geo, physics,
  engineering, dashboards, neuroscience, astronomy, medical, volume, and embeddings. Synthetic,
  simulated, generated, or real data is allowed when the example is honest about it.
- Regression/golden: owned by tests and fixture infrastructure, not by a public examples lane.
- Stress/benchmark: keep in flat `examples/c/lab/` unless a dedicated benchmark suite is created.

Use metadata tags, not public folder names, for `workflow`, `scientific`, `real-data`, `simulated`,
`fake-data`, `interactive`, `offscreen`, `compute`, and domain labels. Existing `workflows`,
`scientific`, and `composites` source directories are transitional and may be indexed into the three
public categories by the documentation generator.

Existing examples may be reorganized aggressively to match these lanes. Prefer fewer, stronger
public examples over many scripts with unclear public roles:

1. keep tiny one-feature examples, but label them as fixtures or visual-family examples rather than
   the public story;
2. promote `protein.c`, `lidar.c`, `brain.c`/`ibl_brain.c`, `labels.c`, `scatter_axes.c`,
   `image_probe.c`, `linked_panels.c`, and WebGPU fixtures into the v0.4 proof set where they still
   match the current API;
3. add a retained textured-mesh terrain/planet example as a v0.4 showcase once the feature slice
   lands;
4. add or polish a weather/wind-field example as the main 2D field showcase;
5. add a composed explanatory-layout example combining axes, colorbar, categorical legend, scale
   bar, annotation/readout, and panel reserves;
6. keep GUI examples as integration examples unless they directly support a showcase;
7. add scenario IDs and metadata for gallery-critical examples before broad documentation
   migration;
8. for every selected showcase, record the missing feature list and explicitly mark deferred pieces
   in the example notes.


## Proposed Layout

Datoviz:

```text
examples/
  c/
    visuals/
    features/
    composites/
    showcases/
    lab/
    legacy/
  python/
    visuals/
    features/
    showcases/
    regression/
data/
  examples/
testing/
  fixtures/
```

vispy2:

```text
examples/
  gsp/
    fundamentals/
    visuals/
    features/
    showcases/
  plot/
    basics/
    linked/
    dashboards/
    scientific/
    gallery/
notebooks/
```


## Scenario IDs

Use stable lowercase IDs so related examples can be tracked across layers:

```text
hello_scene
offscreen_capture
visual_points_basic
feature_point_picking
technique_wboit_mesh
showcase_earth_cubemap
stress_large_scatter
```

When the same scenario exists in several layers, keep the ID and vary only the layer/path. Scenario
IDs should appear in manifests, fixture names, release staging, and cross-repo issue references.


## Metadata Manifests

Each runnable or generated example should eventually have compact metadata:

```yaml
id: showcase_earth_cubemap
title: Earth Cubemap
layer: c | raw_python | gsp | plot | fixture
category: visual | feature | composite | showcase | lab | fixture
tags: [real-data, workflow, offscreen]
status: required | experimental | fixture-only | future | external
data: inline | synthetic | bundled | public-download
validation: smoke | screenshot | readback | fixture | manual
related:
  - c/showcases/earth_cubemap.c
  - vispy2/examples/gsp/showcases/earth_cubemap.py
```

Gallery-critical examples should extend this compact manifest with documentation and asset fields:

```yaml
gallery:
  card: front-page | section | hidden
  title: Earth Cubemap
  summary: Textured planet mesh with lighting, camera animation, and capture.
  image: assets/gallery/showcases/earth_cubemap.webp
  video: assets/gallery/showcases/earth_cubemap.webm
  poster: assets/gallery/showcases/earth_cubemap.webp
  alt: Textured Earth mesh rendered with Datoviz
  capture_command: just capture showcase_earth_cubemap
  docs_page: examples/showcases/earth-cubemap.md
  status_label: supported | experimental | fixture-only | deferred
features:
  - mesh
  - sampled-field
  - texture
  - arcball
  - capture
```

These fields let the documentation generator build visual MkDocs pages without copying example
facts into Markdown by hand. Generated gallery pages should include the canonical media asset,
source/example path, run or capture command, backend requirements, release status, and feature tags.
The metadata should also make it possible to validate that every public gallery card has a
nonblank image, alt text, and a reachable source example.


## Data, Validation, And Duplication Policy

- Follow [POLICIES.md](POLICIES.md) for cache/download rules and avoid repeating them
  in every worked example.
- C public examples should be smoke-testable; regression checks belong in tests; lab stress
  examples need measurable bounds when retained.
- GSP/plot examples may prioritize user-facing clarity but should still expose smoke paths where
  practical.
- Do not duplicate a scenario just to show syntax. Duplicate only when a layer teaches a genuinely
  different user task or validates a different contract.
- Keep docs and examples linked by scenario ID rather than copying long explanations across files.


## Maturity Levels

| Level | Meaning |
| --- | --- |
| sketch | idea captured, not runnable |
| fixture | deterministic validation artifact or generated stream |
| smoke | runnable with bounded data and basic validation |
| documented | suitable for API/docs teaching |
| showcase | polished enough for screenshots and demos |
| stress | intentionally measures capacity/performance |


## Migration Plan

1. Assign scenario IDs to existing examples and specs.
2. Move or index examples into the lane layout without changing unrelated source behavior.
3. Add metadata manifests for high-value examples first.
4. Keep Python binding examples thin in Datoviz.
5. Mirror only selected scenarios into vispy2 GSP/plot once those layers are ready.
6. Promote deterministic smoke/regression examples into CI gradually.


## Initial Python Binding Scope

Start with small, direct examples: hello scene, offscreen capture, bounded window, points, image,
volume slice, mesh, point picking, image probing, partial update, resize, and capture. These should
prove binding conformance, not become a full Python gallery.
