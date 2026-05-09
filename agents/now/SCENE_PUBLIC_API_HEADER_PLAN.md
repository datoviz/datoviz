# Scene Public API Header Plan

> **Execution Status**
> - **Status:** `IMMEDIATE NEXT AGENT TASK`
> - **Updated on:** `2026-05-09`
> - **Purpose:** map the current interaction/text/scale/annotation notes onto concrete public
>   header drafting work.

# Objective

Stop expanding prose and draft the next scene-facing public header surface.

The next agent should turn the spec-side API surface and decision records into concrete API spelling
in public headers and one implementation-facing scratch companion.


## Primary Deliverables

1. Draft the first public API skeleton in [include/datoviz/scene.h](/home/cyrille/GIT/Viz/datoviz/include/datoviz/scene.h)
   or in a small adjacent header split under `include/datoviz/scene/`.
2. Add one implementation-facing companion note or scratch header for concrete typedefs, enums,
   result structs, and ownership notes.
3. Write `2-3` tiny end-to-end usage examples in docs before implementation starts.
4. Only after the header surface feels coherent, begin the narrow first implementation slice.

Normative starting points:

1. [spec/scene/api/API_SURFACE.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/api/API_SURFACE.md)
2. [spec/scene/decisions/README.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/decisions/README.md)
3. [spec/scene/headers/scene_api.h](/home/cyrille/GIT/Viz/datoviz/spec/scene/headers/scene_api.h)


## Header-Drafting Scope

The first public draft should focus on four groups:

1. interaction objects,
2. selection / link / probe result types,
3. scale / colormap / colorbar types,
4. text / annotation handles.

Recommended public-header landing zone:

1. keep [include/datoviz/scene.h](/home/cyrille/GIT/Viz/datoviz/include/datoviz/scene.h) as the
   umbrella include,
2. if the file starts growing too fast, split into focused headers such as:
   `include/datoviz/scene/interaction.h` and `include/datoviz/scene/annotation.h`,
3. keep small public result/descriptor structs in
   [include/datoviz/scene/types.h](/home/cyrille/GIT/Viz/datoviz/include/datoviz/scene/types.h)
   if they need to be visible.

Recommended scratch/companion location:

1. update [spec/scene/headers/scene_api.h](/home/cyrille/GIT/Viz/datoviz/spec/scene/headers/scene_api.h)
   if one authoritative draft header is still the clearest place,
2. or add one narrowly scoped header sketch under `spec/scene/headers/` if that keeps the split
   clearer.


## Decisions To Force In Code Form

The next agent should settle these in the draft header, not in another narrative note:

1. opaque handle style:
   decide which scene-facing objects stay opaque handles and which payloads are exposed as public
   structs,
2. header split:
   decide whether the first draft stays in `scene.h` or moves into a small focused split,
3. lifetime/update conventions:
   decide where explicit create/destroy is required and where panel-owned convenience constructors
   are acceptable,
4. result struct shape:
   decide whether `DvzPickResult` / `DvzProbeResult` are fixed-layout public structs or use a
   tagged/extensible payload pattern,
5. formatting objects:
   decide whether one shared base formatting descriptor is reused across scales, axes, annotations,
   and probes,
6. link key representation:
   decide the public key type and whether channels are named by string, id, or handle.

Suggested bias:

1. scene-owned retained objects use opaque handles,
2. request descriptors and result payloads are public structs,
3. prefer one shared base formatting descriptor with small per-domain extensions,
4. prefer a small header split if `scene.h` becomes difficult to scan.


## Usage Examples To Write Before Coding

The next agent should write tiny docs examples first, because awkward examples are the fastest sign
that the API spelling is wrong.

Required example themes:

1. mesh face or object selection with one link channel,
2. image probe with one pinned readout,
3. scale + colormap + colorbar + annotation label.

The examples can live in:

1. `docs/guide/`,
2. `docs/tasks/<date>-scene-public-api/`,
3. or another short docs location that is easy for later agents to find.

Keep them minimal. They are API-shape probes, not tutorials.


## Narrow Implementation Order After Review

Once the header draft has one review pass, implementation should proceed in this order:

1. interaction core,
2. scale / colormap / colorbar core,
3. text / annotation retained objects,
4. runtime realization details only after the public object model is stable enough.


## Minimum Review Checklist

Before implementation starts, the next agent should verify:

1. the draft names fit existing `Dvz*` naming and module boundaries,
2. public structs are small, stable, and do not leak runtime ownership,
3. the examples do not require hidden callbacks or undocumented global state,
4. the new surface still matches the spec and decision records:
   `spec/scene/api/API_SURFACE.md`,
   `spec/scene/decisions/INTERACTION_API_DESIGN.md`,
   `spec/scene/decisions/ANNOTATION_TEXT_SCALE_API.md`,
   `spec/scene/decisions/PICKING_DESIGN.md`,
   `spec/scene/decisions/PROBE_READOUT_DESIGN.md`,
   `spec/scene/decisions/COLORBAR_COLORMAP_DESIGN.md`,
   `spec/scene/decisions/AXES_DOMAIN_DESIGN.md`.


## Handoff Summary

If a next agent only has time for one concrete step, it should:

1. draft `INTERACTION_API_TYPES`,
2. draft `ANNOTATION_TEXT_SCALE_TYPES`,
3. add minimal function prototypes,
4. request one review pass on the proposed header surface before implementation.
