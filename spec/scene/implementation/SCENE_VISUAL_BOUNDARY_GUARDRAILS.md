# Scene Visual Boundary Guardrails

This note defines the next major visual-architecture refactor phase after the first scene source
split. The goal is not another file-shuffle. The goal is to make generic scene code depend on a
visual-family contract instead of concrete visual families.

Normative status: implementation architecture plan. Public visual semantics remain in
`../semantics/` and `../visuals/`; this file defines internal ownership boundaries, guardrails,
migration order, and done criteria.

This is the single active implementation document for the remaining visual-boundary architecture
work. Earlier broad source-split plans have been retired; completed history stays in git.


## Current State

The first broad scene architecture split is complete enough that future work should not restart it.
In particular:

1. the old flat scene planning bucket has been split across `frame_plan/`, `scene_emit/`, and
   `render_contract/`;
2. normal scene output carries typed visual metadata, and the untyped descriptor compatibility path
   has been removed;
3. `visuals/registry/` owns the private `DvzVisualFamilyOps` table, with active-family coverage
   tests;
4. active family folders own retained lowering, bind descriptors, normal pipeline descriptors,
   shader descriptor bodies, and draw descriptor hooks;
5. image, labels, and volume metadata fill routes through family hooks;
6. normal runtime render preparation is mostly descriptor-driven instead of family-switch-driven;
7. generic dense-attribute, retained-index-buffer, and material-trigger upload emission has moved
   into scene-emission support helpers;
8. many query helpers are now generic where they should be generic: scratch allocation, standard
   item-id decode, standard item-target eligibility, native/sample target fallback policy, and
   render-metadata completeness.

Do not reopen these completed splits unless a regression shows that their current owner boundary is
wrong.

Remaining valuable work is narrower: stop generic scene/visual code from knowing concrete visual
families, and add checks so the coupling does not grow back.

2026-05-30 status correction: the first guardrail pass did not enforce descriptor-kind
independence. In particular, root-level visual helpers still contain concrete
`DVZ_SCENE_VISUAL_DESC_*` matrices. The boundary check now tracks those leaks with a temporary
counted allowlist. Treat that allowlist as the active refactor queue, not as accepted architecture.
Because v0.4-dev does not preserve v0.3 compatibility, legacy untyped/compatibility render paths may
be removed outright when they block the clean split.


## Problem

The current visual layout has active family folders and a private `DvzVisualFamilyOps` registry, but
some generic files still contain concrete family branches. This keeps the old coupling alive:
adding or changing a visual can still require edits in root visual helpers, scene emission, query,
or runtime-adjacent code.

The target architecture should make visual-specific behavior live in:

1. `src/scene/visuals/<family>/`;
2. explicit shared visual subsystems such as `src/scene/visuals/stroke/`;
3. the single declarative family registration table.

Generic scene code should orchestrate work. It should not decide how point, mesh, image, volume,
path, text, or any other concrete visual behaves.


## Target Shape

The long-term retained visual object should hold common state plus an opaque family payload:

```c
typedef struct DvzVisual
{
    DvzScene* scene;
    const DvzVisualFamilyOps* ops;
    void* family_state;

    bool visible;
    int32_t z_layer;
    DvzAlphaMode alpha_mode;
    bool depth_test_enabled;

    /* Shared bindings and cross-family metadata only. */
} DvzVisual;
```

Only family code should cast `family_state`. Generic code should call callbacks:

```c
visual->ops->bounds(visual, &bounds);
visual->ops->metadata(visual, &metadata);
visual->ops->build_uploads(visual, &uploads);
visual->ops->shader_desc(&desc, picking, wboit, format_tag, &shader);
visual->ops->draw_desc(&desc, &draw);
```

The family contract should grow only when a real generic switch is being removed. Candidate
responsibilities:

1. lifecycle defaults, reset, and family-state destruction;
2. attribute schema, aliases, dense-data validation, and source policy;
3. bounds and generated-bounds data;
4. derived geometry, texture/cache payloads, and upload payload construction;
5. FramePlan visual metadata emission;
6. bind, pipeline, shader, pass-capability, and draw descriptors;
7. query target capability, temporary query geometry, and result decoding;
8. diagnostics for family-specific contract failures.

Keep orchestration outside family ops. `scene_emit/` still owns resource-key allocation, upload-node
ordering, pass scheduling, and dependency graph construction. `runtime/` still owns DRP2 emission,
render target realization, viewport/scissor setup, descriptor refresh, and graph-resource
execution.

Uploads are resource writes, not rendering decisions. Texture-backed visuals must still reach
runtime emission through typed visual metadata, visual descriptors, and draw contracts. The runtime
must not recover image, glyph, labels, volume, or textured-mesh rendering from texture uploads,
resource names, or upload order.

Non-visual cleanup such as coarse scene CMake targets, broad private-header shrinking, and remaining
domain/annotation ownership polish should use this same rule: move only when a stable owner is clear,
and do not duplicate completed source-split work. Those items are secondary to the visual-boundary
guardrails in this phase.


## Bounds Local-Transform Contract

The current bounds path still has one important transitional exception: generic
`src/scene/visuals/bounds.c` knows that sphere bounds already resolve local-transform semantics.
This exception exists to preserve correct sphere and bounds-overlay behavior. Sphere impostor bounds
are not a normal transformed AABB: the sphere family transforms item centers and scales radii by the
maximum local-transform axis length, matching shader-side impostor behavior. Applying the generic
AABB-corner transform afterward would double-apply or distort those bounds and can regress the
generated bounds overlay.

The long-term fix is not to keep a concrete `DVZ_VISUAL_TYPE_SPHERE` branch in generic bounds code.
Instead, the visual-family registry should expose a neutral bounds contract, such as:

```c
bool bounds_resolve_local_transform;
```

or an equivalent named policy field/callback on `DvzVisualFamilyOps`. Generic bounds code should
then apply its default local-transform AABB step only when the family contract says the reducer
returned pre-transform bounds. Sphere would opt out because its family reducer already returns
local-transform-aware bounds. Future impostor, billboard, glyph, splat, vector, or custom-shader
families can make the same decision through the registry contract without adding another generic
family branch.

This refactor must be behavior-preserving and should not be bundled with unrelated bounds-overlay
changes. Before removing the temporary allowlist entry for the sphere branch, validate at least:

```sh
just test scene/scene-graph/visual_bounds_family_reducers
just test scene/scene-graph/panel_visual_bounds_sphere_local_transform_screen
just test scene/scene-graph/panel_bounds_overlay_sphere_wire_padding
just test scene/scene-graph/panel_bounds_overlay_sphere_multi_radius_bounds
just spec-check
git diff --check
```

If these tests expose a behavioral change, keep the explicit allowlist entry and defer the registry
contract until the sphere overlay path can be hardened separately.


## Boundary Rules

Generic code may use these things:

1. `DvzVisualFamilyOps` and registry lookup helpers;
2. generic capability bits, descriptor structs, and metadata structs;
3. generic visual categories only when they come from registry flags or descriptor fields;
4. shared helper APIs in explicitly shared subsystems such as `visuals/stroke/`.

Generic code must not:

1. switch on concrete `DVZ_VISUAL_TYPE_*` values for normal visual behavior;
2. include family-private headers such as `point/internal.h` or `volume/internal.h`;
3. inspect family-specific state fields on `DvzVisual`;
4. infer visual families from resource order, upload order, shader names, or descriptor-kind lists;
5. select render behavior from resource names, upload resource kind, or texture presence;
6. add a root-level switch when a new registry callback or descriptor field would express the
   contract.

Allowed central knowledge:

1. public enum declarations and public constructors;
2. the declarative registration table;
3. tests, examples, and specs;
4. temporary allowlisted migration files, with a cleanup owner and removal condition.


## Mechanical Guardrails

Add a repository check, for example `tools/check_scene_visual_boundaries.sh`, and wire it into a
focused validation recipe before treating this phase as complete.

The check should fail when concrete visual-family names appear in generic implementation paths:

```sh
VISUAL_ENUM_RE='DVZ_VISUAL_TYPE_'
VISUAL_ENUM_RE="${VISUAL_ENUM_RE}(POINT|PIXEL|MARKER|SEGMENT|PATH|IMAGE|MESH|VOLUME)"
VISUAL_ENUM_RE="${VISUAL_ENUM_RE}|DVZ_VISUAL_TYPE_"
VISUAL_ENUM_RE="${VISUAL_ENUM_RE}(PRIMITIVE|SPHERE|GLYPH|TEXT|LABELS|SPLAT|VECTOR)"

rg \
  "${VISUAL_ENUM_RE}" \
  src/scene/scene_emit \
  src/scene/runtime \
  src/scene/render_contract \
  src/scene/query \
  src/scene/visuals/*.c
```

It should also fail when family-private headers leak outside family folders and the registration
table:

```sh
rg '#include "([a-z_]+/internal\.h)"' src/scene \
  --glob '!src/scene/visuals/*/*' \
  --glob '!src/scene/visuals/registry/registry.c'
```

Start with an explicit allowlist for current transitional files. The allowlist must shrink as each
migration slice lands. Do not add new allowlist entries without recording why the generic layer
cannot express the behavior through a contract yet.

The guard must also enforce descriptor-kind ownership:

1. concrete `DVZ_SCENE_VISUAL_DESC_*` identifiers may appear in the enum declaration, the
   registration table, tests/examples/specs, and the owning family folder;
2. for example, `DVZ_SCENE_VISUAL_DESC_SEGMENT` belongs in `src/scene/visuals/segment/` and the
   registry, not in root visual helpers or generic runtime code;
3. explicit shared subsystems should consume neutral facts such as renderable kind, point-like kind,
   draw-contract bits, or callback outputs rather than naming concrete descriptor kinds;
4. temporary exceptions live in `tools/scene_visual_boundary_allowlist.txt` as counted entries, so
   new leaks fail and removed leaks require the allowlist to shrink.

Recommended architecture tests:

1. every active visual type has a registry entry and required callbacks;
2. normal scene render visuals always carry typed metadata;
3. generic runtime files do not reference concrete visual-family symbols;
4. generic scene-emission files call family operations instead of family switches;
5. generic query orchestration uses query family ops for family-specific payload/result behavior;
6. generic visual root files contain registry dispatch or shared defaults, not concrete-family
   behavior.


## Migration Order

Work in small behavior-preserving slices.

1. Inventory current leaks.
   - Generate the initial allowlist from current `DVZ_VISUAL_TYPE_*`, `DVZ_SCENE_VISUAL_DESC_*`,
     and family-private include occurrences.
   - Classify each occurrence as registration, public constructor, test/spec, shared default,
     temporary compatibility, or behavior leak.
2. Move attribute schema ownership.
   - Move family attribute tables, aliases, expected-attribute diagnostics, and dense-data
     validation from root visual helpers into family callbacks.
3. Move descriptor construction ownership.
   - Shrink `visuals/desc.c` toward typed metadata validation and registry dispatch.
   - Family code should translate family metadata into bind/pipeline/shader/draw-ready contracts.
   - Remove descriptor-kind matrices from root `desc.c`, `desc_kind.c`, `shader_desc.c`,
     `pipeline_desc.c`, and `pass_caps.c`; keep only neutral helpers that do not know concrete
     built-in families.
4. Move lifecycle and family state ownership.
   - Stop growing `DvzVisual` with family-specific fields.
   - Introduce opaque family state for new or migrated families, then move existing family state
     gradually.
5. Move remaining upload/cache builders.
   - Keep upload orchestration in `scene_emit/`.
   - Move pure derived geometry, texture payload, lookup payload, and cache construction into
     family folders or explicit shared subsystems.
6. Move remaining query ownership.
   - Keep request queue, framebuffer coordinate policy, readback routing, and standard item-id
     decode generic.
   - Move family scratch geometry, non-item result decoding, and unsupported-target decisions into
     family query hooks.
7. Enforce the checks.
   - Wire the boundary script into the normal scene architecture validation path.
   - Remove allowlist entries after each completed migration.


## Done Criteria

The phase is complete when:

1. adding a visual family requires editing only `src/scene/visuals/<family>/`, the registration
   table, and tests/examples/specs;
2. generic scene emission, runtime emission, query orchestration, render-contract validation, and
   root visual helpers do not branch on concrete visual families for normal behavior;
3. generic files do not include family-private headers;
4. family-specific retained state is opaque or has a clear migration plan with no new fields added
   to generic `DvzVisual`;
5. visual-specific behavior is confined to family folders or explicit shared visual subsystems;
6. texture-backed visuals have no runtime fallback path that bypasses typed metadata and draw
   contracts;
7. architecture checks enforce the boundary in CI or the default validation workflow;
8. full scene validation passes after the allowlist reaches the agreed final state.
