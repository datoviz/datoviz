# Napari Integration Notes

Status: index / routing guide
Authority: informative only

This directory contains napari-specific integration pressure tests. These files do not define scene
semantics, public Datoviz API names, or runtime contracts on their own.


## Files

1. [NAPARI.md](NAPARI.md): current v0.4 napari adapter pressure test. Start here for ownership
   boundaries, layer mapping, minimal milestone, and open Datoviz decisions.
2. [NAPARI_POC.md](NAPARI_POC.md): retired v0.3 offscreen canvas-replacement proof-of-concept note.
   Keep it historical; do not use it as v0.4 implementation guidance.


## Canonical Sources

Use these specs for implementation rules rather than repeating generic prose here:

1. [../HOSTED_BACKENDS.md](../HOSTED_BACKENDS.md): hosted event loops and external surfaces.
2. [../EXTERNAL_UI.md](../EXTERNAL_UI.md): host-owned UI boundaries.
3. [../HIGH_DPI.md](../HIGH_DPI.md): logical and physical pixel behavior.
4. [../THREAD_SAFETY.md](../THREAD_SAFETY.md): cross-thread handoff.
5. [../../pipeline/RESOURCE_MODEL.md](../../pipeline/RESOURCE_MODEL.md): sampled fields, ownership,
   dirty ranges, and dirty regions.
6. [../../pipeline/FRAME_PLAN.md](../../pipeline/FRAME_PLAN.md): per-frame execution planning.
7. [../../pipeline/TRANSFORM_PIPELINE.md](../../pipeline/TRANSFORM_PIPELINE.md): coordinate and
   normalization rules.
8. [../../interaction/PICKING.md](../../interaction/PICKING.md): picking and readout semantics.
