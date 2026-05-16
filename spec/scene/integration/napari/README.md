# Napari Integration Notes

This directory contains napari-facing integration guidance.

These files are informative. They do not define scene semantics, public API names, or runtime
contracts on their own.


## Files

1. [NAPARI.md](NAPARI.md): current v0.4 design guidance for a future napari adapter.
2. [NAPARI_POC.md](NAPARI_POC.md): historical v0.3 offscreen proof-of-concept note.


## Authority

Use the specialized scene specs for implementation rules:

1. [../HOSTED_BACKENDS.md](../HOSTED_BACKENDS.md) for hosted event-loop and external-surface
   integration.
2. [../HIGH_DPI.md](../HIGH_DPI.md) for logical and physical pixel behavior.
3. [../EXTERNAL_UI.md](../EXTERNAL_UI.md) for host-owned UI boundaries.
4. [../../pipeline/RESOURCE_MODEL.md](../../pipeline/RESOURCE_MODEL.md) for sampled fields,
   resource ownership, and dirty updates.
