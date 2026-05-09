# Deferred Items Tracker

This document consolidates all explicitly deferred items across the scene and DRP2 specs.
It is informative — the authoritative deferral note lives in the originating document.

Items are grouped by target milestone.


## DRP2 2.1

These items are deferred at the DRP2 protocol level.

| Item | Source |
|---|---|
| Level 3 output conformance (cross-backend pixel parity) | `spec/drp2/CONFORMANCE.md` |
| GPU execution harness for Level 2 mechanical verification | `spec/drp2/CONFORMANCE.md` |
| Remaining deferred DRP2 commands (lower priority) | `spec/drp2/schema/DEFERRED.md` |


## v0.4+

These items are in scope for v0.4 but deferred beyond the initial implementation target.

| Item | Source |
|---|---|
| Per-panel render scale for image export | `spec/scene/export/IMAGE_EXPORT.md` |
| Lanczos and high-quality downsampling filters for export | `spec/scene/export/IMAGE_EXPORT.md` |
| GPU compute pre-pass for non-linear projection (e.g., geographic) | `spec/scene/semantics/NONLINEAR_TRANSFORMS.md` |
| Non-linear transform interaction with axes | `spec/scene/semantics/NONLINEAR_TRANSFORMS.md` |
| Tapered (per-vertex width) lines | `spec/scene/visuals/PATH.md` |
| `volume` multiplane / MPR render mode | `spec/scene/visuals/VOLUME.md` |
| Automatic LOD for mesh simplification | `spec/scene/semantics/GEOMETRY_UTILITIES.md` |
| Concave (alpha-shape) hull computation | `spec/scene/semantics/GEOMETRY_UTILITIES.md` |
| Annotation shape generation (msdfgen integration) | `spec/scene/semantics/GEOMETRY_UTILITIES.md` |
| Scene-level animation timeline coordination | `spec/scene/interaction/ANIMATION.md` |
| Multi-scene GPU resource sharing across threads | `spec/scene/integration/THREAD_SAFETY.md` |
| Polar axis geometry (circular gridlines, radial labels) | `spec/scene/pipeline/TRANSFORM_PIPELINE.md` |
| Map tile loading and LOD management | `spec/scene/pipeline/TRANSFORM_PIPELINE.md` |
| Per-item material / PBR lighting for sphere | `spec/scene/visuals/SPHERE.md` |
| Selection state synchronization across scenes | `spec/scene/interaction/SELECTION.md` |
| Custom visual dirty-tracking optimization | `spec/scene/integration/CUSTOM_VISUALS.md` |
| 3D text (per-character orientation) | `spec/scene/visuals/GLYPH.md` |
| Volume picking (ray-cast identity) | `spec/scene/interaction/PICKING.md` |
| Vector export tile-based rendering and SVG-level effects | `spec/scene/export/VECTOR_EXPORT.md` |
| `wiggle` promotion to a full visual family | `spec/scene/../semantics/VISUAL_FAMILIES.md` |


## No Target Set

These items have no assigned milestone and are indefinitely deferred.

| Item | Source |
|---|---|
| Hardware ray tracing capability class | `spec/scene/validation/ADAPTATION.md` |
| `RayTraceNode` plan node kind | `spec/scene/pipeline/FRAME_PLAN.md` |
