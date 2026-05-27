# Datoviz And vispy2 Example Suite Organization

> **Status:** planning
> **Scope:** Datoviz v0.4 C examples, raw Python examples, vispy2 GSP/plot examples, fixtures,
> showcases, regressions, and stress examples
> **Goal:** keep examples discoverable by purpose and API layer while sharing scenario IDs

This document defines repository ownership and layout. Release staging lives in
[EXAMPLE_RELEASE_STAGING.md](EXAMPLE_RELEASE_STAGING.md); one-feature fixture ideas live in
[FEATURE_FIXTURE_MATRIX.md](FEATURE_FIXTURE_MATRIX.md). Shared worked-example policy lives in
[SHARED_POLICIES.md](SHARED_POLICIES.md).


## Goals

- Split examples by purpose first, then by API layer.
- Keep C examples as canonical native engine/API examples.
- Keep raw Python examples in this repository as thin Datoviz binding examples.
- Keep GSP and plot examples in vispy2, where Python-native workflows belong.
- Separate fixtures, documentation examples, showcases, regressions, and stress tests.
- Share stable scenario IDs across repositories.


## Repository Ownership

- Datoviz owns C public API examples, raw Python binding examples, engine fixtures,
  deterministic regression examples, native stress tests, native showcases, and generated
  DRP2/DVZR/WebGPU fixtures. It should not become the main high-level Python gallery once GSP/plot
  are active.
- vispy2 owns GSP object-oriented scene examples, plot-interface examples, Python showcases,
  notebooks, napari/Qt/dashboard integrations, and NumPy/pandas/xarray/SciPy workflows. It should
  not own native engine conformance examples or low-level runtime fixtures.

Raw Python in Datoviz should stay close to the Datoviz API. If an example needs rich Python object
ownership, plotting conveniences, notebooks, or ecosystem data loading, it belongs in vispy2.


## API Layers

- C, in `examples/c/`: canonical engine, scene, app, DRP2, vklite/canvas, interop, offscreen,
  regression, and stress examples.
- Raw Python, in `examples/python/`: thin binding smoke/parity examples that load the library,
  create scenes, upload data, render offscreen/windowed, pick/probe/update/capture.
- GSP, in vispy2: object-oriented scene programming, retained visuals, callbacks, selections,
  linked scene state, and Pythonic animation/update patterns.
- Plot, in vispy2: task-first scatter/line/image/volume/surface/mesh plots, linked views,
  dashboards, scales, annotations, labels, and selections.


## Example Lanes

- Fundamentals: create/render/update/close workflows with small synthetic data, such as hello
  scene, offscreen capture, bounded window, resize, frame callback, animation, multi-panel, and
  linked panzoom.
- Visuals: one active visual family at a time, such as point, pixel, marker, primitive, segment,
  path, mesh, image, volume slice/rendering, sphere impostor, and text/annotation when active.
- Features: reusable scene/app capabilities, such as partial update, mutability hints, visibility,
  depth, controllers, sampled fields, colormaps, colorbar, picking, probing, selection, links,
  pinned readout, video export, and DVZR.
- Techniques: pass-level behavior, such as alpha blending, WBOIT, EDL, SSAO, MSAA, materials,
  transparency, postprocess, and depth/ordering variants.
- Advanced/low-level: DRP2 streams, vklite/canvas/stream/video, interop, offscreen, and
  command/resource lifecycle.
- Showcases: polished scientific/domain demos for geo, physics, engineering, dashboards,
  neuroscience, astronomy, medical, volume, and embeddings.
- Regression/golden: deterministic screenshots, readbacks, fixture streams, and schema/generated
  output parity.
- Stress/benchmark: capacity and performance examples for large scatter, image grids, streaming,
  repeated updates, descriptor/resource churn, and live-loop frame pacing.

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
    fundamentals/
    visuals/
    features/
    techniques/
    advanced/
    showcases/
    regression/
    stress/
  python/
    fundamentals/
    visuals/
    features/
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
lane: fundamentals | visuals | features | techniques | advanced | showcase | regression | stress
status: required | experimental | fixture-only | future | external
data: inline | synthetic | bundled | public-download
validation: smoke | screenshot | readback | fixture | manual
related:
  - c/showcases/earth_cubemap.c
  - vispy2/examples/gsp/showcases/earth_cubemap.py
```


## Data, Validation, And Duplication Policy

- Follow [SHARED_POLICIES.md](SHARED_POLICIES.md) for cache/download rules and avoid repeating them
  in every worked example.
- C examples should be smoke-testable; regression examples need deterministic outputs; stress
  examples need measurable bounds.
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
4. Keep raw Python examples thin in Datoviz.
5. Mirror only selected scenarios into vispy2 GSP/plot once those layers are ready.
6. Promote deterministic smoke/regression examples into CI gradually.


## Initial Raw Python Scope

Start with small, direct examples: hello scene, offscreen capture, bounded window, points, image,
volume slice, mesh, point picking, image probing, partial update, resize, and capture. These should
prove binding conformance, not become a full Python gallery.
