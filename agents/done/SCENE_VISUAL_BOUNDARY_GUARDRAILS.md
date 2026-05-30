# Scene Visual Boundary Guardrails

Status: completed baseline on 2026-05-29; superseded on 2026-05-30 by stricter descriptor-kind
independence work.

2026-05-30 correction: this baseline blocked several visual-type and private-header leaks, but it
did not block concrete `DVZ_SCENE_VISUAL_DESC_*` references in root visual helpers and generic
runtime code. That remaining debt is tracked by
[`../../tools/scene_visual_boundary_allowlist.txt`](../../tools/scene_visual_boundary_allowlist.txt)
and the active rules in
[`../../spec/scene/implementation/SCENE_VISUAL_BOUNDARY_GUARDRAILS.md`](../../spec/scene/implementation/SCENE_VISUAL_BOUNDARY_GUARDRAILS.md).
Do not treat the 2026-05-29 baseline as final completion of visual independence.

The scene visual-boundary phase moved generic visual behavior onto registry-owned family
operations and added a default validation guard so concrete visual-family coupling does not grow
back in generic scene paths.


## Landed Shape

1. `DvzVisual` now carries common retained visual state plus `ops` and opaque `family_state`.
2. Family operations own lifecycle hooks, attribute validation side effects, schema/alias tables,
   render metadata, descriptor kind selection, material policy, panel clipping policy, scale
   capability, and sampled-field texture-upload policy.
3. Root visual and generic scene paths dispatch through the registry instead of normal-behavior
   switches over concrete `DVZ_VISUAL_TYPE_*` families.
4. Family-private headers are confined to family folders, explicit bridge/shared subsystem headers,
   and the registry table.
5. Query dense-attribute access lives with visual query geometry rather than generic query scratch
   orchestration.
6. Visual release paths tolerate already-reset family state for buffer, sampled-field, and binding
   teardown.


## Guardrails

The phase added:

1. `tools/check_scene_visual_boundaries.py`
2. `tools/scene_visual_boundary_allowlist.txt`
3. `just spec-check` integration for the scene visual boundary guard

The allowlist is empty except comments at completion. Future exceptions should be temporary,
documented in the allowlist, and removed as soon as a registry callback or shared subsystem can
express the contract.


## Validation

Completion validation:

1. `git diff --check`
2. `python3 tools/check_scene_visual_boundaries.py`
3. `just build`
4. `direnv exec . just test scene` - `508/508` passed
5. `just spec-check` - DRP2 fixtures `124/124`, WebGPU preflight fixtures `39/39`, fixture tests
   `52` passed, WebGPU preflight tests `19` passed, scheduler tests `5` passed, scene query source
   guard `1` passed, scene architecture source guard `1` passed, and scene visual boundary guard
   passed


## Follow-Up Rule

Before adding a visual family, root-level visual switch, family-private include, or new
family-specific retained field, read
[`../../spec/scene/implementation/SCENE_VISUAL_BOUNDARY_GUARDRAILS.md`](../../spec/scene/implementation/SCENE_VISUAL_BOUNDARY_GUARDRAILS.md).
The default expectation is that new behavior belongs in `src/scene/visuals/<family>/`, an explicit
shared visual subsystem, or the registry contract.
