# Scene Historical Header Sketches

This directory contains historical header sketches derived from earlier scene-spec passes.

They were used to pressure-test:

1. naming,
2. handle and descriptor boundaries,
3. enum surfaces,
4. diagnostics payload shape,
5. the scene-to-runtime split.


## Normative Status

Installed headers under `include/datoviz/scene*.h` are authoritative for public names and
signatures that already exist.

`diagnostics.h` and any runtime-service sketches are informative unless a later document explicitly
promotes them.

These files are not:

1. installed headers,
2. compiled headers,
3. frozen public API,
4. independent sources of scene semantics.

Public API policy now lives in
[../api/API_SURFACE.md](../api/API_SURFACE.md). Prefer editing installed headers plus tests for
active work.


## Files

1. `scene_public_api_draft.h`: compact companion note for the installed interaction, scales,
   colorbars, text, and annotations header split
2. `diagnostics.h`: draft shared diagnostic enums and record/report types


## Source Specs

These sketches are derived from:

1. `../api/API_SURFACE.md`
2. `../validation/DIAGNOSTICS.md`
3. `../api/IMPLEMENTATION_NOTES.md`
