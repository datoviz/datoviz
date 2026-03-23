# Example: Volume Offscreen Rendering And Export

This example pressure-tests the `volume` family together with offscreen targets and deterministic
readback.


## Scene Setup

1. one scene,
2. one offscreen panel or export-oriented virtual panel,
3. one `volume` visual,
4. one volumetric sampled field,
5. one export request for deterministic readback.


## Family And Variant

Family:

1. `volume`

Variant axes:

1. direct or colormap mode,
2. traversal/compositing mode,
3. optional quality mode.


## Resource Schema Instance

Scene-facing resources:

1. source volumetric `SampledField`,
2. `StyleBlock` for transfer and traversal controls,
3. offscreen `DerivedField`,
4. `ReadbackTarget` for the exported image.


## Transform Pipeline

1. volume-domain semantics originate in `DataSpace`,
2. visual-ready volume framing is derived in `VisualSpace`,
3. offscreen panel-local camera state views that volume,
4. resulting rendered output is captured through an offscreen/readback path.


## FramePlan Shape

Typical export frame:

1. `UploadNode` when volume data or transfer settings changed,
2. one `RenderNode` targeting an offscreen derived target,
3. one `ReadbackNode` for deterministic export.

If a future volume variant requires preprocessing:

1. a `ComputeNode` may appear before the render node.


## DRP2 Categories Implied

1. resource writes for volume data and style blocks,
2. render-pass lifecycle for the offscreen target,
3. draw commands,
4. copy or readback path for export,
5. queue submission.


## Pressure On The Spec

This example checks that:

1. offscreen rendering remains independent from window state,
2. `volume` semantics stay distinct from `image`,
3. readback is modeled explicitly in `FramePlan`,
4. deterministic export fits the same family and transform model as onscreen rendering.
