# Scene Header Sketches

This directory contains draft headers derived from the current scene spec.

They are intended to pressure-test:

1. naming,
2. handle and descriptor boundaries,
3. enum surfaces,
4. diagnostics payload shape,
5. the scene-to-runtime split.


## Normative Status

Installed headers under `include/datoviz/scene*.h` are now authoritative for public names and
signatures that already exist. `scene_api.h` is a legacy auxiliary draft for older or broader API
ideas not yet promoted to installed headers; do not treat it as overriding installed headers.
Several names in that file intentionally remain stale because they preserve design history.

`diagnostics.h` and any runtime-service sketches are informative unless a later document explicitly
promotes them.

These files are not:

1. installed headers,
2. compiled headers,
3. frozen public API,
4. independent sources of scene semantics.

Public API policy now lives in
[../api/API_SURFACE.md](../api/API_SURFACE.md). Keep these sketches aligned with installed headers
when preserving them for design history; prefer editing installed headers plus tests for active work.


## Files

1. `scene_api.h`: legacy broad auxiliary draft C spelling for older scene API ideas not yet
   promoted; not a source of truth for active public names
2. `scene_public_api_draft.h`: compact companion note for the installed interaction, scales,
   colorbars, text, and annotations header split
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
