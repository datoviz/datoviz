> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-09`
> - **Purpose:** consolidate the active v0.4 normalization and precision decisions for scientific
>   coordinates across mesh, point, image, and volume workflows.

# Scientific Coordinate Normalization

This note consolidates the active normalization decisions that are already present across the scene
spec, with emphasis on the 3D scientific-coordinate cases that matter now.


## Objective

Make the active contract explicit for workflows where source data lives in meaningful scientific
coordinates and must still render robustly:

1. mesh and point data in large or high-precision coordinate systems,
2. volumes with real-world bounds,
3. shared 3D framing across several visual families,
4. picking and readback in original semantic coordinates.


## Existing Grounding In The Repo

The underlying policy already exists in the broader scene spec:

1. transform pipeline:
   [spec/scene/pipeline/TRANSFORM_PIPELINE.md](../../pipeline/TRANSFORM_PIPELINE.md)
2. resource precision policy:
   [spec/scene/pipeline/RESOURCE_MODEL.md](../../pipeline/RESOURCE_MODEL.md)
3. axes/domain semantics:
   [spec/scene/proposals/active/AXES_DOMAIN_DESIGN.md](../active/AXES_DOMAIN_DESIGN.md)
4. volume framing pressure:
   [spec/scene/proposals/active/VOLUME_DESIGN.md](../active/VOLUME_DESIGN.md)

This note is not a new transform model. It is an active consolidation of the decisions that matter
for implementation.


## Core Recommendation

Normalization of scientific coordinates is a scene concern, and it happens before panel navigation.

Recommended split:

1. source coordinates live in semantic scientific `DataSpace`,
2. scene normalizes them in CPU-side F64 into renderable `VisualSpace`,
3. panel camera/panzoom/controller transforms apply afterward,
4. F32 downcast happens only at upload time after normalization.

This is already the right policy in the repo and should be treated as active contract.


## What Is Already Settled

The following decisions are already effectively made and should not be reopened casually:

1. Stage A normalization belongs above DRP2,
2. Stage B panel navigation belongs below normalization,
3. CPU-side normalization uses F64,
4. F32 downcast happens only after normalization,
5. camera movement or panzoom should usually not force renormalization,
6. normalized resources should be reusable across panels when practical.

Those points are already the backbone of `TRANSFORM_PIPELINE.md`.


## Why This Matters For 3D Scientific Scenes

The active 3D roadmap now needs the same discipline as the 2D spec already had.

Typical case:

1. a brain surface mesh,
2. interior region meshes,
3. one volume with mm-space bounds,
4. one slice plane through the volume,
5. picking and readback that must report meaningful scientific coordinates.

If these objects are not normalized coherently, they may render together accidentally but will not
behave coherently under picking, probing, and linked annotations.


## Shared 3D Normalization Frame

For mixed 3D scenes, the important decision is that related visuals should be able to share one
scientific normalization frame.

Recommended rule:

1. when several visuals represent one coherent scientific scene, they should be normalized into one
   shared `VisualSpace` frame,
2. mesh, volume, slice, and point overlays should align in that frame,
3. panel controllers then operate on that shared renderable frame.

This is the right model for anatomy/atlas-style scenes.


## Per-Visual Versus Shared Normalization

Not every visual needs to share one normalization frame, but the distinction should be explicit.

Recommended policy:

1. use shared normalization when visuals are semantically in the same world,
2. use per-visual normalization when they are independent analytic objects,
3. do not let this be accidental.

That means the scene may need an explicit notion of a shared spatial frame or normalization group
later, even if the first implementation keeps the API narrow.


## 2D And 3D Split

The underlying normalization principles are the same in 2D and 3D, but the active pressures differ.

2D emphasis:

1. panel domains,
2. panzoom stability,
3. axes and scale bars.

3D emphasis:

1. shared world frame across mesh and volume,
2. camera/controller independence from normalization,
3. semantic pick/probe coordinate reporting.


## Picking And Coordinate Readback

Normalization is not complete unless readback semantics are clear.

Recommended rule:

1. picking and probing should be able to report original scientific coordinates,
2. normalized/render-space coordinates may exist internally but are not the primary scientific
   answer,
3. mixed scenes should round-trip from rendered hit back to semantic source coordinates consistently,
4. probe/readout APIs should recover semantic coordinates by default rather than exposing
   normalization-space coordinates as the primary user-facing answer.

For slice picking this means:

1. panel pixel
2. slice-local coordinate
3. world/scientific coordinate
4. sampled value

For mesh picking this means:

1. visual identity
2. optional instance/face identity
3. optional world/scientific hit position later


## Volume Alignment

Volumes are one of the strongest reasons to keep this policy explicit.

Recommended behavior:

1. volume bounds are defined in original scientific units,
2. the scene aligns the volume into the same normalized 3D frame as related meshes,
3. slice planes are defined in the same semantic frame,
4. probe readback returns semantic coordinates from that frame.


## Axes And Domain Relationship

In 2D, axes own visible semantic domain interpretation.

In 3D, the analogous need is not full 3D axes first, but consistent semantic coordinate framing
for:

1. scale references,
2. dimension labels,
3. probe readouts,
4. bounding-box overlays.

This is one place where axes/domain and annotation work meet the normalization policy.


## Precision Policy

The repo’s current F64 policy should be treated as mandatory for scientific-coordinate ingestion and
normalization.

Recommended summary:

1. source position data may be F64,
2. geometry utility work stays in F64,
3. normalization stays in F64,
4. F32 reaches the GPU only after values are well-conditioned in `VisualSpace`.

This should remain true for:

1. mesh geometry,
2. point/scatter positions,
3. volume bounds and slice placement,
4. derived annotation anchors.

Implementation caution:

1. avoid per-visual hidden normalization conventions,
2. related visuals should declare or inherit shared semantic framing explicitly when they belong to
   one scientific scene.


## What Still Needs A Sharper API

Although the underlying policy exists, a few API-level questions still deserve follow-up:

1. how users declare that several visuals share one scientific normalization frame,
2. how the scene exposes original coordinate readback in pick/probe results,
3. whether 3D normalization groups should be explicit scene objects,
4. how external UI probes or inspectors query that semantic frame.

The policy is already there. The user-facing API around it is what still needs sharpening.


## Immediate Scope Recommendation

The active implementation should assume:

1. CPU-side F64 normalization is not optional,
2. mesh and volume work may need one shared 3D normalization frame,
3. pick/probe results should be designed to carry semantic coordinates,
4. camera/controller work must remain separate from normalization.


## Explicit Non-Goals For This Note

1. replacing `TRANSFORM_PIPELINE.md`,
2. redefining all 2D panel-domain semantics,
3. introducing backend-specific large-world-coordinate tricks into the public scene API,
4. forcing every visual in a scene into one shared normalization frame.
