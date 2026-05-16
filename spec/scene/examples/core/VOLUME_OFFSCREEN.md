# Example: Volume Offscreen Rendering And Export

> **Agent Pickup**
> - **Category:** `core`
> - **Implementation target:** Small runnable C example or focused scene/DRP2 regression on the active v0.4 path.
> - **Data policy:** Inline or deterministic synthetic data unless this file explicitly names a cache.
> - **Preprocessing:** None for the first slice; keep any later generator deterministic and checked in or documented.
> - **Validation:** Bounded smoke run plus screenshot/readback or fixture coverage when practical.


This example pressure-tests the `volume` family together with offscreen targets and deterministic
readback.


## Owning Specs

This example should be read against:

1. `../../semantics/VISUAL_FAMILY_RULES.md` for the `volume` family contract,
2. `../../pipeline/RESOURCE_MODEL.md` for volumetric fields and readback targets,
3. `../../core/RUNTIME_BOUNDARY.md` for offscreen completion semantics,
4. `../../pipeline/FRAME_PLAN.md` for offscreen render and readback participation.


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
2. `ParameterBlockResource` for transfer and traversal controls,
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
2. any compute-produced intermediate should be frame-local by default unless the scene explicitly
   declares a reusable derived cache.


## DRP2 Categories Implied

1. resource writes for volume data and parameter blocks,
2. render-pass lifecycle for the offscreen target,
3. draw commands,
4. copy or readback path for export,
5. queue submission.


## Pressure On The Spec

This example checks that:

1. offscreen rendering remains independent from window state,
2. `volume` semantics stay distinct from `image`,
3. readback is modeled explicitly in `FramePlan`,
4. deterministic export fits the same family and transform model as onscreen rendering,
5. compute-assisted preprocessing does not silently become persistent authoritative scene state.
