# Render Products Refactor Handoff

Status: **implementation complete through R8; final convergence active**. Integration branch: `orchestrate/rc3-render-qa`. Updated: 2026-08-02.

R9 cleanup, documentation, exact-head validation, and integration are tracked by [RC3_RENDER_PRODUCTS_LANDING.md](RC3_RENDER_PRODUCTS_LANDING.md) and [RC3_RENDER_PRODUCTS_AFFECTED_QA.md](RC3_RENDER_PRODUCTS_AFFECTED_QA.md). This file preserves execution history and packet acceptance criteria; it is not current semantic authority.

This handoff translated [the approved architecture](../../spec/scene/proposals/promoted/RENDER_PRODUCTS_AND_TECHNIQUE_COMPOSITION.md) into the R0-R9 implementation campaign. The maintainer authorized local implementation and checkpoint commits on 2026-08-02; pushes and publication remain governed by the current user authorization and repository rules.

## Required Reading And Authority

1. Read `AGENTS.md`, `agents/now/START.md`, `agents/now/STATUS.md`, and this file.
2. Read `spec/scene/README.md`, `spec/scene/AUTHORITY.md`, `spec/scene/implementation/GRAPH_TECHNIQUES.md`, `spec/scene/implementation/OCCLUSION_EFFECTS.md`, `spec/scene/implementation/TRANSPARENCY_MSAA.md`, `spec/scene/semantics/EFFECTS.md`, and the architecture proposal above.
3. Read `spec/drp2/README.md`, `spec/drp2/AUTHORITY.md`, and `spec/drp2/READING_ORDER.md` before any DRP2 change.
4. Read `spec/bindings/README.md`, `spec/bindings/CTYPES_POLICY.md`, and `spec/bindings/ctypes.yml` before the public API checkpoint.
5. Treat specialized approved specs as authoritative. If the maintainer approves this proposal, promote its accepted decisions into specialized specs during packet R0 before runtime edits.

## Campaign Entry Gate

The maintainer completed this gate on 2026-08-02 by approving:

1. approved or edited every decision in the proposal's Maintainer Decision Gate;
2. selected the implementation base branch and exact base commit;
3. approved a separate render implementation branch or worktree;
4. confirmed whether a narrow issue #137 stabilization patch should precede the architecture migration or be delivered inside R7;
5. accepted the checkpoint and QA integration sequence in [HANDOFF_RC3_RENDER_QA_ORCHESTRATION.md](HANDOFF_RC3_RENDER_QA_ORCHESTRATION.md).

The maintainer also approved the R0 cycle-resolution refinement on 2026-08-02: AO uses a coherent surface-capture prepass before `surface_analysis` and forward opaque shading, and scene occlusion uses a typed `scene_occlusion_depth` product distinct from `surface_depth` and `volume_first_hit_depth`.

Implementation uses a fresh branch from the integrated approved base with no standalone blur-only patch. Add characterization tests first, then deliver the estimator and architectural fix through R7. A future emergency narrow fix requires a new maintainer request.

## Non-Negotiable Invariants

1. Scene remains the semantic planner; DRP2 remains the backend-neutral renderer protocol; vklite remains the Vulkan runtime; canvas/stream/app retain presentation ownership.
2. Product identity is typed and plan-local. Human labels, resource suffixes, technique names, shader names, ordinal positions, and upload order are not identities.
3. Semantic product contracts are separate from physical graph resources and carry panel/view scope, validity, coordinate/color/depth/normal/alpha semantics, sample domain, and legal consumers.
4. Surface depth, normal, and coverage describe the same winning opaque or masked fragment after clipping, discard, analytic sphere intersection, corrected depth, and sample resolution.
5. Visual order and render-phase order remain separate; source-over and overlay order cannot change accidentally.
6. AO modulates ambient or indirect diffuse only. It does not darken direct light, specular, emissive, unlit, transparent, volume, overlay, query, or presentation contributions.
7. EDL consumes AO-aware opaque color and canonical surface depth before transparency. Transparent and volume work do not produce or consume AO in RC3.
8. MSAA transitions are explicit and product-specific. Depth, normals, coverage, and IDs are never silently averaged as color.
9. Query and scientific scalar semantics bypass presentation effects unless an approved query contract states otherwise.
10. Runtime code never destroys, resets, transitions, submits, or otherwise assumes ownership of borrowed handles beyond its contract.
11. Capability fallback is explicit and diagnostic. It may reduce quality but cannot silently reinterpret product semantics or disable an enabled technique.
12. Legacy and replacement composition paths do not survive together at campaign completion.

## Current Removal Targets

The campaign removes or replaces these coupled mechanisms:

1. effect classification spread through `src/scene/scene_emit/panel_render_plan.{c,h}` and `src/scene/scene_emit/panel.c`;
2. effect-specific graph orchestration in `src/scene/techniques/{graph_gbuffer,graph_ssao,graph_wboit,graph_depth_peel}.c`;
3. `DvzFramePlanRenderPassRole` and the role-to-`work_label` policy as runtime composition identity;
4. `(panel_id, work_label, ordinal)` graph matching in `src/scene/render_contract/**`;
5. `SceneEdlTargets`, `SceneSsaoTargets`, `SceneWboitTargets`, and `SceneDepthPeelTargets` buckets plus effect-role dispatch in `src/scene/runtime/**`;
6. effect-private `_bg_*` interpretation in `src/app/trace.c` and its tests;
7. the SSAO hit-only denominator, fixed pixel blur contract, and black-overlay composite shaders;
8. figure-sized intermediate assumptions where products are semantically panel-local.

Effect-specific shader and pipeline providers may remain when they are selected through typed declarative work contracts rather than runtime family switches.

## Dependency DAG

```text
R0 authority + characterization
  -> R1 product contracts and graph schema
      -> R2 composer, layers, phases, immutable snapshots
          -> R3 declarative work and generic lowering
              -> R5 coherent surface record and semantic resolves
                  -> R7 GTAO, denoise, material AO
              -> R6 EDL and transparency migration
          -> R4 panel-local coordinates and relative allocation
              -> R5, R6, R7
      -> R8 public API and binding migration after semantics freeze
R5 + R6 + R7 + R8
  -> R9 cleanup, conformance, promotion, and landing manifest
```

R4 may begin after R2 only when its schema is frozen and its writer does not overlap R3. R8 may draft API deltas after R2, but generated bindings and final documentation wait until R5 and R7 freeze user-visible behavior.

## R0 — Freeze Authority And Characterize Behavior

Owner: main `sol-medium` orchestrator or a supervised `terra-medium` test mapper. Checkpoint: `scene: freeze render-product refactor invariants`.

Paths:

- approved proposal promotion into `spec/scene/implementation/**`, `spec/scene/semantics/EFFECTS.md`, `spec/scene/pipeline/**`, and affected visual specs;
- characterization tests in `src/scene/tests/{scene_techniques,frame_plan,frame_plan_emit,scene_graph,app}.c` and `src/scene/tests/visuals/runtime.c`;
- issue #137 examples and capture fixtures without committing unapproved binary media.

Deliverables:

1. Promote approved semantic product vocabulary, phase order, participation policy, AO lighting rule, resolve rule, panel coordinate rule, and deferred scope into durable specs.
2. Add non-brittle fixtures for current graph topology and draw ordering that do not enshrine work-label identity.
3. Add failing or characterization coverage for AO zoom sweeps, isolated and dense analytic spheres, meshes, silhouettes, stationary redraw stability, perspective/orthographic projection, multi-panel offsets, resize, and MSAA off/on.
4. Record numeric image metrics and tolerances separately from exact graph/JSON fixtures.

Acceptance:

- Current behavior tests pass before refactoring, except intentionally failing issue #137 quality assertions clearly marked as the target.
- Fixtures distinguish current output characterization from the approved new semantic expectation.
- No DRP2, runtime, shader, or public API changes occur in this packet.

Stop and escalate if the approved decisions cannot be expressed consistently across existing specialized specs, or if a required fixture would commit prohibited binary data.

R0 evidence on the Linux RTX 5090 runner: all 14 focused SSAO tests pass; the perspective FOV sweep at `0.45`, `0.75`, and `1.05` records visibility minima `136`, `130`, and `186` with means `244.799`, `251.647`, and `253.453`; the legacy blur minimum is `203`; orthographic minimum is `185`; MSAA 1x and 4x captures are identical on this backend; stationary redraw, unequal-panel isolation, and resize round-trip have maximum delta `0`; strict panel-local background has zero dark pixels. `just build`, `just spec-check` including all 125 DRP2 fixtures and WebGPU checks, and `git diff --check` pass. These are characterization values, not R7 quality thresholds.

## R1 — Typed Products And Frame-Plan Schema

Owner: main `sol-medium`; do not delegate the data-model decision. Checkpoint: `scene: add typed semantic render products`.

Primary paths:

- `src/scene/frame_plan/{frame_plan.h,internal.h,core.c,nodes.c,resources.c,passes.c,dependencies.c,capabilities.c,diagnostics.c,json.c,ascii.c,fixture.c}`;
- `src/scene/frame_plan/graph/**`;
- `src/scene/render_contract/**` as read/adaptation consumers;
- `include/datoviz/scene/frame_plan.h` only if inspection is intentionally public.

Deliverables:

1. Add immutable internal `RenderProductContract` data with typed product IDs, semantic kind, panel/view identity, camera/projection identity, extent and coordinate transform, format class, sample domain/count, resolve policy, encoding, validity, alpha/coverage, access, producer, consumers, and lifetime.
2. Keep physical graph resource descriptors separate and link them explicitly to product versions.
3. Validate producer uniqueness, compatible consumption, panel isolation, same-pass feedback, read-before-produce, sample transitions, background validity, and product/resource compatibility.
4. Expose deterministic semantic provenance in ASCII, JSON, diagnostics, and fixtures.

Acceptance:

- Focused frame-plan and graph tests reject cross-panel reuse, incompatible depth/normal pairing, implicit sample changes, undefined background reads, and semantic inference from resource format alone.
- Existing plans can be represented without changing rendered output.
- Product IDs are not exported as stable user handles.

Stop and escalate if existing fixed graph limits cannot represent the needed attachments/accesses, if product identity would need persistence across plans, or if a protocol capability appears necessary. Any DRP2 change starts with authoritative prose, schema, and fixtures.

R1 implementation evidence: typed plan-local products and coherent surface-record identities remain internal to the scene FramePlan; growable typed use records bind consumers and validity requirements without a fixed consumer limit; physical resources and diagnostic labels remain non-authoritative realizations; exact attachment and explicit shader resolves, format classes, access closure, panel scope, surface pairing, validity payloads, live intervals, and aliases are mechanically validated. JSON schema `0.2` and ASCII output expose deterministic provenance. The focused product matrix passes 15/15, the broader frame-plan lane passes 88/88, `just spec-check` passes all 125 DRP2 fixtures, 39 WebGPU fixtures/streams, pytest gates, and source guards, and `git diff --check` passes. No DRP2 protocol change was required.

## R2 — Central Composer, Layers, Phases, And Snapshots

Owner: main `sol-medium`; cheap agents may map visual capabilities read-only. Checkpoint: `scene: centralize panel technique composition`.

Primary paths:

- `src/scene/scene_emit/panel_render_plan.{c,h}`, `src/scene/scene_emit/panel.c`, and related emission internals;
- `src/scene/techniques/{core,state}.c` plus new focused composer modules;
- `src/scene/render_contract/{core,visual,resources}.c`;
- declarative capability facts in `src/scene/visuals/**/{lowering,pipeline,shader}.c` and visual metadata.

Deliverables:

1. Classify each draw into one semantic visual layer and retain its authored order independently of graph phase order.
2. Add immutable internal technique contracts for inputs, outputs, phase constraints, visual participation, capabilities/fallbacks, and pass expansion.
3. Perform one per-panel required-product closure, producer selection, capability resolution, graph expansion, validation, and immutable snapshot transaction.
4. Store resolved pass and draw contracts so render validation and lowering consume the same facts.
5. Remove `(panel_id, work_label, ordinal)` as render-contract matching identity.

Acceptance:

- Identical retained state and capability snapshots produce identical product/pass order and diagnostics.
- Overlapping blended visual order, labels, annotations, and fixed-controller content retain authored semantics.
- Visual families declare capabilities but do not build technique graphs or switch on technique names.
- Missing producers, ambiguous producers, phase cycles, and incompatible participation fail precisely.

Stop and escalate when a visual needs technique-family behavior that cannot be represented as a layer, phase, product use, material feature, ordered draw fact, or capability. Define the missing semantic contract instead of adding another family switch.

R2 implementation evidence: visual families now declare baseline semantic layers, product capabilities, and phase participation; generated annotation, card, bounds, text, and glyph roles resolve explicitly to overlay while fixed backgrounds remain surface content; one centralized immutable contract table selects ordered technique instances, resolves product-version chains and complete capabilities/fallbacks, expands generic pass templates, validates producer uniqueness, cycles, phase constraints, pass identities, persisted contract drift, complete physical binding, and exact pass-template expansion, and publishes the exact panel snapshot transactionally into the FramePlan. Repeated source-over transforms retain distinct technique and `scene_color` versions, compatible volume source-over work preserves authored order in transparent shading, overlays compose last, semantic occlusion dependencies remain explicit across transparency algorithms, and unsupported noncontiguous repeated OIT runs fail rather than persist false order. Render nodes and graph passes use typed panel-local composition identities plus direct indices, draw contracts are frozen from retained metadata, MSAA emission consumes the snapshot decision, and the checkpoint-local label bridge is confined to final legacy graph binding for removal during R3. Full scene-graph coverage passes 215/215, including GPU SSAO/WBOIT/depth-peel paths, focused composition/capability/overlay/persistence/identity tests pass, all 88 affected FramePlan tests pass, `just spec-check` passes, and `git diff --check` passes.

## R3 — Declarative Work And Generic Lowering

Owner: main `sol-medium`; subagents may mechanically migrate isolated providers only after the contract freezes. Checkpoint: `scene: lower composed render products generically`.

Primary paths:

- `src/scene/runtime/{render_emit,render_emit_prepare,render_emit_passes,render_emit_draws,render_emit_bindings,technique_targets,graph_resources,render_pass,state}.c`;
- `src/scene/render_contract/drp2.c`;
- technique shader/pipeline providers and focused runtime tests.

Deliverables:

1. Add declarative work payloads for visual draw batches, fullscreen draws, and compute dispatches, including provider key, product bindings, attachments, clear/load/store, viewport/scissor/coordinate transform, sample/resolve policy, draw filter/order, and diagnostics.
2. Realize graph resources, attachments, descriptors, pipelines, and ordered work from typed contracts.
3. Remove effect target buckets and runtime role/label lookup as each migrated path becomes representable.
4. Preserve DRP2 pass order, explicit transitions, descriptor refresh, failure transactionality, and borrowed target ownership.

Acceptance:

- An existing provider can service a new composed pass without adding an effect bucket or suffix lookup to runtime code.
- Runtime lowering does not interpret product names, work labels, or technique-family enums.
- Focused scene-to-DRP2 and runtime-vklite tests prove setup/update/frame packet behavior and failure rollback.

Stop and escalate if the lowerer cannot choose a pipeline, binding, draw set, load/store action, or coordinate transform without scanning a name. Enrich the typed work contract; do not encode a convention in runtime.

## R4 — Panel-Local Coordinates And Relative Allocation

Owner: supervised `terra-medium` after R2 schema freeze. Checkpoint A: `scene: make technique coordinates panel local`. Checkpoint B: `scene: realize technique products per panel`.

Primary paths:

- product/graph extent and validation code under `src/scene/frame_plan/**`;
- runtime resource creation and descriptor refresh under `src/scene/runtime/**`;
- technique shaders that currently use global `gl_FragCoord` or figure extent;
- multi-panel, unequal-panel, HiDPI, resize, and aliasing tests.

Deliverables:

1. Carry panel pixel origin, local extent, render scale, and local-to-target transform through products and pass uniforms.
2. Convert sampling to panel-local coordinates and prove no cross-panel read while physical allocations may temporarily remain figure-sized.
3. Switch intermediates to panel-relative or source-relative allocations after coordinate parity passes.
4. Recreate only invalidated resources and bind groups on panel resize; permit aliasing only for non-overlapping compatible lifetimes.

Acceptance:

- Two unequal adjacent panels may enable different techniques without cross-talk.
- Non-integral layout, HiDPI scale, minimized/zero extent, resize, and per-panel render scale are validated.
- No technique intermediate remains figure-sized without an explicit approved reason.

Stop and escalate if the current runtime cannot attach subextent resources, if resource pooling crosses active panel scope, or if resize would violate borrowed presentation ownership.

## R5 — Coherent Surface Record And Semantic MSAA Resolve

Owner: main `sol-medium`; this is a correctness boundary. Checkpoint: `scene: make surface records and multisample resolves semantic`.

Primary paths:

- surface/G-buffer graph and visual participation paths;
- sphere, mesh, primitive, point, and masked visual shader/lowering paths that are approved surface producers;
- frame-plan validation and runtime resource/pipeline lowering;
- DRP2 and vklite only if current attachment/resolve fields cannot express the approved contract.

Deliverables:

1. Produce one coherent nearest-surface record for eligible opaque/masked visuals, including analytic sphere ray hit, fragment depth, normal, clipping/discard, coverage, and background validity.
2. Declare surface color/depth/normal/coverage multisample products and their single-sample consumer products.
3. Resolve depth, normal, and coverage as one winning-surface policy; never pair minimum depth with an unrelated averaged normal.
4. Keep alpha-to-coverage a material/draw capability and object IDs integer-selected, not filtered.

Acceptance:

- Sphere and mesh silhouettes, intersections, alpha-to-coverage, MSAA off/on, perspective/orthographic projection, and background edges preserve coherent records.
- Pipeline and attachment sample counts match; sampled consumers receive explicitly resolved products.
- Any DRP2 delta has prose, schema, fixtures, native tests, and WebGPU preflight updated before scene consumes it.

Stop and escalate for unsupported depth resolve modes/formats, an unrepresentable same-surface selection rule, or a visual whose capture shader cannot match its shaded fragment semantics.

## R6 — EDL And Transparency Migration

Owner: main `sol-medium`; `terra-medium` may migrate trace expectations after semantics freeze. Checkpoint: `scene: compose EDL and transparency through products`.

Primary paths:

- `src/scene/techniques/{graph_gbuffer,graph_wboit,graph_depth_peel}.c` and replacement recipes;
- runtime effect branches/providers;
- source-over, WBOIT, depth-peel, volume-occlusion, EDL shaders and tests;
- `src/app/trace.c` and `src/app/tests/test_app.c`.

Deliverables:

1. Express EDL as `surface_depth + AO-aware opaque scene_color -> scene_color` in `surface_postprocess`.
2. Express source-over, WBOIT, and depth peeling as `transparent_shading` recipes with explicit color, depth, accumulation, transmittance, load/store, alpha, and ordering contracts.
3. Preserve authored source-over order and explicit transparent algorithm semantics while excluding transparent/volume work from AO.
4. Remove private `_bg_edl_*`, `_bg_wboit_*`, and peel-name semantics from trace normalization.

Acceptance:

- AO+EDL+each transparency mode combinations match the approved order: opaque material AO, EDL, transparent composition, volume, overlay, presentation.
- Disabled techniques allocate no private targets or force passes on unaffected panels.
- Existing WBOIT and peel blend/dependency correctness tests survive as semantic assertions.

Stop and escalate if a requested combination contradicts the approved phase order or needs transparent AO. Do not preserve current incidental insertion order as compatibility behavior.

## R7 — Deterministic GTAO, Denoise, And Material Integration

Owner: main `sol-medium`; shader implementation may use a bounded `terra-medium` subagent after equations and ABI are frozen. Checkpoint A: `scene: produce stable ambient visibility`. Checkpoint B: `scene: integrate ambient visibility in material lighting`.

Primary paths:

- `src/scene/shaders/glsl/{ssao,ssao_blur,ssao_composite}.frag` and governed WGSL/registry/generated shader inputs;
- new GTAO/denoise shaders and uniforms;
- `src/scene/techniques/{state,graph_ssao}.c` and replacement recipe;
- shared material lighting include plus sphere/mesh/primitive and other eligible lit-family providers;
- shader registry, ABI checks, scene/runtime/render tests, and issue #137 examples.

Deliverables:

1. Replace hit-only normalization with a bounded horizon-based view-space visibility estimator whose background/border samples remain unoccluded members of the declared domain.
2. Use deterministic directions with no temporal jitter, semantic view-space radius/thickness/falloff/bias, and correct perspective/orthographic reconstruction.
3. Produce `ambient_visibility`, then edge-aware denoise/reconstruct it using coherent depth/normal/coverage and a projection-derived footprint.
4. Add one shared material-lighting hook and modulate only ambient/indirect diffuse for eligible lit opaque/masked visuals.
5. Remove ordinary black-overlay composition; keep a side-effect-free AO debug presentation path.

Acceptance:

- Issue #137 zoom sweeps contain no black-dot outliers beyond approved thresholds.
- Fixed inputs are stable across redraws; cross-resolution/backend results satisfy documented tolerant metrics.
- AO remains finite and bounded at background, viewport borders, silhouettes, clipped surfaces, and degenerate projection cases.
- Direct, specular, emissive, unlit, transparent, volume, overlays, queries, and scalar color mapping remain unchanged except where explicitly approved.
- Shader ABI/registry checks and representative validated offscreen captures pass.

Stop and escalate if eligible visual families lack a common ambient/indirect material term, if shader ABI would need per-family duplication, or if capability fallback would silently revert to black composition.

## R8 — Public API Break And Bindings

Owner: main `sol-medium` for API; supervised `terra-medium` for mechanical binding/docs updates. Checkpoint: `scene: expose semantic ambient-occlusion controls`.

Primary paths:

- `include/datoviz/scene/{scene.h,types.h,enums.h}` and exported implementations;
- examples and API tests using EDL, SSAO, or MSAA;
- `spec/scene/api/API_SURFACE.md`, specialized technique specs, public docs if affected;
- `spec/bindings/ctypes.yml`, generated metadata/bindings through repository commands, and binding smokes.

Deliverables:

1. Replace the ordinary SSAO descriptor with approved semantic appearance controls: radius, intensity, thickness or bias, quality, optional minimum visibility, and debug mode.
2. Remove sample count, raw pixel blur radius, blur toggle, and blur sigma from the stable surface or place them only in an explicitly unstable expert/debug descriptor if approved.
3. Remove superseded pass-role or technique API that exposes old architecture; do not expose internal product IDs or raw graph mutation.
4. Update C examples, exact ctypes policy/layout decisions, generated binding, Python call sites, and public documentation together.

Acceptance:

- `just ctypes`, `just ctypes-check`, `just ctypes-smoke`, and `just docs-api-check` pass before Python/GSP/package validation.
- No stale exported declarations, enum documentation, generated bindings, or examples remain.
- The API maps directly to semantic behavior and explicit capability diagnostics.

Stop and escalate if the proposed public surface requires backend handles, raw product/resource IDs, graph-pass mutation, or compatibility scaffolding that preserves both architectures.

## R9 — Cleanup, Conformance, Promotion, And Landing Manifest

Owner: main `sol-medium` integrator; cheap agents may do bounded source-guard and stale-reference sweeps. Checkpoint: `scene: complete render-product technique migration`.

Primary paths: every migrated path, relevant tests/tools/specs, and [QA_SOURCE_AUDIT.md](QA_SOURCE_AUDIT.md).

Deliverables:

1. Delete legacy role-driven composition, work-label matching, effect target buckets, obsolete graph builders, SSAO composite shader, stale trace normalization, compatibility shims, and unused generated shader entries.
2. Add source guards against runtime technique-name/role switches, resource-suffix semantic inference, and visual-family technique graph construction.
3. Promote accepted proposal material into specialized specs, update active proposal routing/status, and remove contradictory prose.
4. Produce a landing manifest with exact base/head commits, changed paths, public/shared header deltas, generated inputs, protocol/schema deltas, ownership/lifecycle changes, test configuration changes, and explicit unaffected claims.
5. Hand the manifest to the QA lane so only mechanically invalidated slices are repeated.

Acceptance matrix:

```sh
just build
just test scene
just test drp2
just test vklite
just test app
just drp2-fixtures
just spec-check
just ctypes
just ctypes-check
git diff --check
```

Also run focused shader ABI/source guards, validated offscreen render conformance, issue #137 zoom captures, multi-panel/resize/MSAA/EDL/transparency combinations, and bounded live presentation only when WSI or presentation ownership changed. Record exact passes, skips, timeouts, provider/GPU/driver identities, and limitations; do not infer unavailable platform proof.

Stop and escalate if a legacy branch remains used by query/readback, WebGPU, trace/replay, or runtime execution. Migrate the consumer before deletion and do not leave a parallel compatibility path.

## Subagent Packet Contract

Every delegated packet must state:

1. exact base commit and assigned paths;
2. immutable architecture decisions and required specs;
3. allowed edits and explicit exclusions;
4. deliverable and acceptance commands;
5. stop/escalation conditions;
6. prohibition on staging, committing, pushing, GitHub mutation, shared generated outputs, and concurrent heavy builds unless the orchestrator explicitly grants the action;
7. required final report: files inspected/changed, findings, tests, limitations, and clean status.

Use `terra-low` for searches, inventories, stale-reference cleanup, test enumeration, and deterministic mechanical edits. Use `terra-medium` for bounded test additions, shader translation after GLSL/ABI freeze, trace migration, panel-coordinate plumbing after schema freeze, and independent review. Keep product schema, composer semantics, material/AO rules, resolve policy, public API, integration, and commits with the `sol-medium` orchestrator. Escalate architecture deviations to a fresh maintainer review; execution subagents do not improvise them.

## Completion Definition

The render lane is complete only when all approved packets pass, no legacy parallel composition path remains, the landing manifest is complete, the affected QA rerun has accepted or fixed the new code, and the final integration matrix has been run from the exact candidate commit. A visually improved SSAO shader alone does not complete this campaign.
