# Scene Visual Boundary Contract

Status: active architecture contract. This replaces the completed implementation queue formerly
tracked in `../implementation/SCENE_VISUAL_BOUNDARY_GUARDRAILS.md`.

The scene visual boundary is closed for v0.4 when generic scene code depends on registry-declared
visual-family contracts, descriptor metadata, and explicit shared visual subsystems rather than
concrete visual-family behavior.


## Final Shape

Visual-family behavior lives in:

1. `src/scene/visuals/<family>/`;
2. explicit shared visual subsystems such as `src/scene/visuals/stroke/`;
3. `src/scene/visuals/registry/`, which is the single declarative table for family ops,
   descriptor kinds, default renderable kind, attribute schema, and family capability flags.

Generic scene code may orchestrate scene emission, resource-key allocation, upload ordering, pass
scheduling, runtime handoff, and query/readback routing. It must not decide how a concrete visual
family behaves.

`DvzVisual` owns common retained state plus an opaque `family_state` pointer. The current v0.4
payload type is `DvzVisualFamilyState`: a private shared payload for material parameters, bindings,
texture/cache state, text realization state, and active family state. This is accepted as the final
v0.4 boundary because family behavior is reached through ops, family folders, explicit shared
subsystems, or known lifecycle/emission bridges. Do not add new family-specific fields or direct
payload accesses from new generic files; add a registry callback, descriptor field, shared visual
subsystem, or family-owned helper instead.


## Hard Rules

Generic code must not:

1. switch on concrete `DVZ_VISUAL_TYPE_*` values for normal visual behavior;
2. include family-private headers such as `point/internal.h` or `volume/internal.h`;
3. inspect family-specific retained state from new generic scene files;
4. infer render behavior from resource order, upload order, shader names, texture presence, or
   descriptor-kind lists;
5. select texture-backed rendering from upload resource names or texture writes.

Allowed central knowledge:

1. public enum declarations and public constructors;
2. `src/scene/visuals/registry/`;
3. tests, examples, and specs;
4. explicit shared subsystems, including stroke, annotation/text realization, scene-emission
   bridges, and sampled-field binding bridges.


## State Payload Ownership

Direct `DvzVisualFamilyState`, `_visual_family_state()`, or `family_state` access is confined to:

1. visual family folders and shared visual subsystems under `src/scene/visuals/`;
2. scene-emission bridge files under `src/scene/scene_emit/`;
3. annotation/text realization owners under `src/scene/annotation/` and `src/scene/text/`;
4. selected scene core/domain bridge files named in `tools/check_scene_visual_boundaries.py`;
5. tests.

New code outside those owners must use public/internal helper APIs or add a registry/ops contract.


## Validation

The boundary is enforced by:

```sh
python3 tools/check_scene_visual_boundaries.py
just spec-check
```

`tools/scene_visual_boundary_allowlist.txt` must remain empty except comments. If a new visual
family needs behavior that does not fit the current contracts, add the smallest neutral callback,
descriptor field, or shared subsystem needed for that behavior and update focused tests.
