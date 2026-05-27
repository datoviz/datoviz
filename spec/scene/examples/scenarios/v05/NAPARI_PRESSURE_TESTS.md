# v0.5 Napari-Class Pressure Tests

> **Example status:** v0.5 pressure tests
> **Target:** Datoviz renderer fixtures plus external napari/GSP integration examples
> **Data:** small prepared caches with deterministic fallbacks
> **Validation:** smoke, linked-view behavior, probing/selection checks

These scenarios pressure Datoviz primitives that napari-like applications need. Full napari
application UX belongs above Datoviz.


## `large_labels_segmentation`

Needs integer label textures, direct GPU categorical sampling, sparse/high-id label handling,
selection styling, categorical legends, and hover/click payloads.


## `multiview_linked_orthoslices`

Needs shared 3D texture slices, linked crosshairs, volume/slice probes, slice dragging, and
multi-panel layout polish.


## `volume_clipping_3d`

Needs richer volume clipping/probing, transfer controls, UI integration, and screenshot/video
capture that shows the clipped interior clearly.


## `gpu_ai_segmentation_interop`

Mostly later/external. Datoviz can keep renderer-level fixtures for probability maps, masks, and
label overlays, but external GPU/AI interop and interactive refinement belong above the scene core.
