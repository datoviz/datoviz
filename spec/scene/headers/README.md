# Scene Header Sketches

This directory contains draft headers derived from the current scene spec.

They are intended to pressure-test:

1. naming,
2. handle and descriptor boundaries,
3. enum surfaces,
4. diagnostics payload shape,
5. the scene-to-runtime split.


## Normative Status

`scene_api.h` is the authoritative draft C spelling for the scene concepts it already covers.
It is still a draft: it is not installed or compiled, and it may change during the v0.4 refactor.

For API groups not yet spelled in `scene_api.h`, [../api/API_SURFACE.md](../api/API_SURFACE.md)
is the normative source until the header sketch is updated.

`diagnostics.h` and any runtime-service sketches are informative unless a later document explicitly
promotes them.

These files are not:

1. installed headers,
2. compiled headers,
3. frozen public API,
4. independent sources of scene semantics.

Public API policy now lives in
[../api/API_SURFACE.md](../api/API_SURFACE.md). Keep
`scene_api.h` aligned with that document when drafting interaction, scale, colorbar, text, and
annotation handles.


## Files

1. `scene_api.h`: authoritative draft C spelling for the handles, descriptors, enums, and entry points it covers
2. `scene_public_api_draft.h`: focused companion draft for interaction, scales, colorbars, text, and annotations
3. `diagnostics.h`: draft shared diagnostic enums and record/report types

Runtime-facing capability, submission, completion, and diagnostic sketches currently live in
`scene_api.h` until a separate header is useful.


## Source Specs

These sketches are derived from:

1. `../api/API_DESIGN.md`
2. `../api/API_SURFACE.md`
3. `../core/RUNTIME_BOUNDARY.md`
4. `../validation/DIAGNOSTICS.md`
5. `../api/IMPLEMENTATION_NOTES.md`
