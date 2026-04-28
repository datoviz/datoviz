# Scene Header Sketches

This directory contains non-authoritative draft headers derived from the current scene spec.

They are intended to pressure-test:

1. naming,
2. handle and descriptor boundaries,
3. enum surfaces,
4. diagnostics payload shape,
5. the scene-to-runtime split.


## Normative Status

These files are informative.

They are not:

1. installed headers,
2. compiled headers,
3. frozen public API.


## Files

1. `scene_api.h`: draft scene-facing handles, descriptors, enums, and entry points
2. `runtime_service.h`: draft runtime-facing capability, submission, completion, and diagnostic surface
3. `diagnostics.h`: draft shared diagnostic enums and record/report types


## Source Specs

These sketches are derived from:

1. `../API_DESIGN.md`
2. `../RUNTIME_BOUNDARY.md`
3. `../DIAGNOSTICS.md`
4. `../IMPLEMENTATION_NOTES.md`
