> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-09`
> - **Purpose:** define the intended active v0.4 volume direction, especially for mixed scenes with
>   transparent meshes, interior opaque meshes, movable slice planes, and precise picking.

# Volume Design

This note narrows the broader `volume` family material into the active design questions that matter
for the current roadmap and your stated use cases.


## Objective

Support a volume workflow that can coexist cleanly with the active 3D mesh work:

1. one 3D volume in a shared scientific coordinate frame,
2. transparent and opaque meshes overlaid with the volume,
3. movable slice planes through the volume,
4. precise picking on the slice and on meshes,
5. retained scene state for slice position, transfer settings, and probe/readout state.


## Existing Grounding In The Repo

There is already substantial background material:

1. family spec:
   [spec/scene/visuals/VOLUME.md](../visuals/VOLUME.md)
2. realistic mixed-use example:
   [spec/scene/examples/MOUSE_BRAIN_ATLAS_EXPLORER.md](../examples/MOUSE_BRAIN_ATLAS_EXPLORER.md)
3. active transparency direction:
   [spec/scene/proposals/TRANSPARENCY_WBOIT_DESIGN.md](TRANSPARENCY_WBOIT_DESIGN.md)
4. active picking direction:
   [spec/scene/proposals/PICKING_DESIGN.md](PICKING_DESIGN.md)

This note turns that background into an active recommendation set.


## Core Recommendation

`volume` should remain its own visual family, and slice display should remain a `volume` render mode,
not an `image` visual pretending to be 3D.

Recommended active modes:

1. `dvr`
2. `slice`

The first active volume implementation does not need every feature in `visuals/VOLUME.md`, but it
should preserve that family boundary now.


## Shared 3D Coordinate Frame

Volume and mesh must be able to live in one shared scientific coordinate system.

Recommended rule:

1. volume bounds are expressed in the same world/scientific frame as mesh geometry after scene
   normalization,
2. slice planes are defined in that same frame,
3. picked positions should be able to round-trip back to meaningful scientific coordinates.

This is the key requirement for brain-volume-plus-region-mesh scenes.


## Slice Planes

Movable slice planes are a first-class requirement.

Recommended baseline:

1. one slice plane controlled by semantic state,
2. axial/sagittal/coronal style axis-aligned slices first,
3. dynamic slice position updates without re-uploading the volume,
4. optional guide-plane annotation in 3D when useful.

Later:

1. arbitrary oblique slice planes
2. multi-slice/MPR layouts

But the first slice should already be a retained scene object or parameter set, not an ad hoc shader
uniform hidden in example code.


## Picking On Volume Slices

Slice picking is important now.

Recommended slice pick result:

1. visual id
2. payload kind = image_pixel or volume_slice_sample
3. slice pixel/cell coordinate
4. corresponding world/scientific coordinate
5. sampled value when available

This is stronger than ordinary 2D image picking because the slice is embedded in a 3D scene and must
still report semantic coordinates.

Recommended interaction rule:

1. slice samples should support both probe-only interaction and optional persistent sample
   selection,
2. when possible, slice readout and selection should reuse the common image-like probe/readout
   structure instead of inventing a separate volume-only query shape.


## Mixed Mesh And Volume Picking

Mixed scenes need explicit policy.

Typical case:

1. transparent outer shell,
2. opaque internal meshes,
3. one slice plane inside the volume.

Recommended policy model:

1. use the general picking hit-selection policy from `PICKING_DESIGN.md`,
2. preserve enough result structure that a query can distinguish:
   - shell hit
   - interior mesh hit
   - slice hit
3. allow `opaque_preferred` and `all_hits_sorted` workflows.

This should not be left to accidental render order.


## Volume And Transparency

Volume rendering and transparent meshes must be planned deliberately together.

Recommended direction:

1. volume participation in transparency/compositing is explicit in the frame plan,
2. transparent meshes and volume passes are not allowed to define one another implicitly,
3. selection and probe overlays remain explicit later-stage annotations.

The exact pass structure can evolve, but the semantics must stay explicit.


## Transfer Functions And Colormaps

Volume rendering should align with the shared scale/colormap architecture where possible.

Recommended direction:

1. scalar volume value-range normalization remains a volume concern,
2. color mapping should be able to reference shared colormap/scale concepts,
3. opacity transfer remains volume-specific semantic state,
4. colorbars should be able to explain slice or DVR mappings without bespoke one-off logic.

This is one reason a dedicated colorbar/colormap note is worth adding next.


## Normalization And Precision

Volume framing is part of the broader scientific-coordinate normalization problem.

Recommended rule:

1. source volume metadata and bounds remain in the original scientific coordinate system,
2. scene normalization aligns the volume and meshes into one shared renderable frame,
3. F64 should remain the authoritative CPU-side coordinate precision before upload/downcast.

This should be tied explicitly to the upcoming normalization note rather than reinvented inside the
volume family.


## Resource Model

Recommended active resource split:

1. one 3D sampled field resource for the source volume,
2. one parameter/state block for slice placement and volume rendering controls,
3. optional derived resources for probe readback or filtered slice outputs later,
4. annotation resources for probe labels or guide planes.

Do not treat a slice as a separately authored 2D image asset by default.


## Controller Implications

Volume scenes increase pressure on controller semantics.

Recommended interactions to support eventually:

1. model-space arcball or camera orbit for 3D inspection,
2. slice-plane movement controller or external-UI-driven slider updates,
3. probe picking on the slice,
4. optional linked 2D views driven by slice state.

The important point is that slice state is scene-owned semantic state, not a backend-private knob.


## Annotation Implications

Volume workflows naturally consume the annotation and text systems.

Useful derived annotations:

1. current slice-position label,
2. probe crosshair or marker,
3. sampled value readout,
4. guide plane outline,
5. optional bounding-box or anatomical reference labels.


## Immediate Scope Recommendation

The narrowest useful active design target is:

1. one `volume` visual family kept distinct from `image`,
2. one slice render mode,
3. shared 3D framing with meshes,
4. slice picking returning semantic coordinates and sampled values,
5. explicit mixed-scene picking policy with transparent meshes,
6. retained scene state for slice position and transfer settings.


## Explicit Non-Goals For The First Active Volume Slice

1. every DVR optimization path,
2. full multi-plane MPR suite immediately,
3. arbitrary oblique slicing in the first pass,
4. ray-traced volume rendering as the baseline path,
5. backend-shaped public APIs for sampling or traversal.
