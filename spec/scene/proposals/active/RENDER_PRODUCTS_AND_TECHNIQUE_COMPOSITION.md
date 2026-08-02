# Render Products And Technique Composition

Status: **approved for RC3 implementation**. The maintainer approved the complete decision gate on 2026-08-02. Updated: 2026-08-02.

This proposal replaces effect-specific scene planning and runtime lowering with a semantic render-product model and a single technique composer. It is deliberately aggressive because v0.4 is still in release-candidate development and API compatibility may be broken to establish a correct long-term boundary.

## Motivation

[GitHub issue #137](https://github.com/datoviz/datoviz/issues/137) reports black dots while zooming the protein and SSAO examples. The issue is labeled `kind:bug` and `area:rendering`; its two comments initially suspect MSAA and then the SSAO blur. Source inspection supports a genuine SSAO defect rather than an expected MSAA artifact.

The current implementation has four coupled failure modes:

1. `ssao.frag` counts only samples that land on geometry and divides occlusion by that geometry-hit count, so a single occluding hit can become nearly black when most samples miss the surface.
2. A small per-pixel rotated kernel creates unstable spatial noise whose footprint becomes visible under magnification.
3. `ssao_blur.frag` uses a fixed screen-pixel footprint even though the AO radius is view-space, so filtering and evaluation diverge as projection and zoom change.
4. `ssao_composite.frag` overlays black on the already-lit scene color, darkening direct light, specular response, and emissive contribution instead of only ambient or indirect illumination.

MSAA can expose or change the edge presentation of these errors, but it does not explain the interior zoom-dependent dots. A narrow blur tweak could hide one symptom while preserving the invalid normalization and lighting composition.

The deeper architectural problem is that SSAO, EDL, WBOIT, depth peeling, and future postprocessing each grow their own graph emitter, pass roles, target structs, string work labels, runtime branches, and attachment assumptions. The renderer lacks a typed vocabulary for what an intermediate image means, so compatibility is inferred from names and effect families instead of validated semantics.

## Goals

1. Make intermediate images first-class semantic render products with mechanically validated compatibility.
2. Separate visual membership, technique composition, frame-graph planning, and backend execution.
3. Make postprocessing consume and produce declared products instead of discovering targets through effect-specific code or string labels.
4. Make SSAO stable under zoom, projection, resolution, MSAA, and mixed materials, and apply it only to the lighting terms it represents.
5. Give EDL, transparency, DoF, bloom, outlines, picking, capture, and future effects a coherent composition boundary.
6. Keep scene planning backend-neutral and lower through the existing DRP2 and vklite runtime path.
7. Permit product-specific capability adaptation without backend forks in scene code.
8. Keep resources panel-local, transient where possible, and eligible for relative-resolution allocation and aliasing.

## Non-Goals For RC3

1. A public plugin ABI or user-editable frame graph.
2. A full deferred renderer or mandatory G-buffer for scenes that do not need one.
3. Temporal accumulation, motion vectors, temporal upscaling, or temporal AO.
4. HDR display negotiation, ICC color management, or a public tone-mapping suite.
5. Ray tracing or a second native rendering runtime.
6. Full WebGPU feature parity; the contract must remain portable and fail or adapt explicitly where the browser runtime is incomplete.
7. Public render-product IDs, raw graph mutation, or exposure of the internal composer as a user-facing API.

## Ownership Boundary

The scene layer owns semantic products, visual-layer classification, technique contracts, composition, and frame-plan emission. DRP2 owns typed backend-neutral resource, pipeline, binding, pass, copy, and submission commands. Vklite owns Vulkan realization and Vulkan resource/command lifetime. Canvas, stream, and app own frame execution and presentation without interpreting scene effects.

Render products are an internal scene planning concept for RC3. They should lower to existing DRP2 resources and passes unless implementation proves a protocol-level semantic or capability gap. Product names, technique names, shader names, and upload order must never become a hidden DRP2 contract.

## Core Model

### Render Products

A render product is a typed logical image or buffer plus the metadata needed to prove that a producer and consumer agree. Its identity is a stable plan-local typed ID, never a string lookup key.

Every product declaration carries:

| Field | Required meaning |
| --- | --- |
| Kind | Semantic role such as scene color, surface depth, surface normal, object ID, or ambient visibility. |
| Domain | Panel, view, scene, query, or presentation scope. RC3 graph effects are panel-local unless explicitly declared otherwise. |
| Extent | Absolute, panel-relative, or source-relative dimensions with a validated scale and rounding policy. |
| Format class | Required channel/type precision independent of a specific backend format; capability resolution selects the concrete format. |
| Samples | Single-sample or multisample count plus whether a resolve is required and which semantic resolve policy applies. |
| Coordinate space | View, world, clip, normalized device, framebuffer pixel, or not applicable. |
| Encoding | Linear depth convention, normal encoding, color transfer/primaries class, integer identity, or scalar range. |
| Alpha | Opaque, straight, premultiplied, transmittance, coverage, or not applicable. |
| Validity | How background, uncovered pixels, partial coverage, and invalid values are represented. |
| Access | Attachment, sampled, storage, transfer, readback, or presentation uses required by the composed graph. |
| Lifetime | Persistent, frame-local transient, history, external, or borrowed. RC3 techniques use frame-local transient products unless a durable owner is explicit. |

The minimum RC3 product kinds are:

| Product | Contract |
| --- | --- |
| `scene_color` | Linear scene-referred color before presentation encoding; alpha semantics are explicit. It may be allocated only when a technique needs an intermediate color target. |
| `surface_depth` | Nearest eligible opaque surface depth with one canonical linear view-space representation for sampling and a declared hardware-depth source if needed for depth testing. Background validity is explicit. |
| `surface_normal` | Normalized view-space surface normal for the same winning surface as `surface_depth`; invalid on background. |
| `surface_coverage` | Coverage or validity for the winning surface, including the resolved-edge rule when MSAA is active. |
| `object_id` | Integer query identity. It is never averaged or filtered and remains outside presentation postprocessing. |
| `ambient_visibility` | Scalar visibility in `[0, 1]`, where `1` is unoccluded. It carries the resolution and reconstruction policy used by material shading. |
| `scene_occlusion_depth` | Nearest depth from explicitly participating scene occluders. It is distinct from the primary opaque surface record and from volume first-hit depth even when a producer contributes or a physical format is shared. |
| `transparent_accumulation` | Technique-private weighted or peeled color/transmittance products with explicit premultiplication semantics. |
| `volume_first_hit_depth` | Volume ray first-hit or occupancy depth. It may share depth conventions with `surface_depth` but is not silently interchangeable with it. |
| `presentation_color` | Final panel color in the transfer function and alpha convention required by the presentation or capture target. |

`surface_depth`, `surface_normal`, and `surface_coverage` form one coherent surface record. Their sample, pixel, and validity correspondence is an invariant. A consumer may not independently select mismatched depth and normal products.

A product contract is distinct from its physical frame-graph resource. The product contract owns meaning, scope, validity, and legal producer/consumer relationships; the graph resource owns allocation, format realization, access, and lifetime. One physical resource may be safely aliased between non-overlapping products, but a format-compatible texture is never enough to prove semantic compatibility.

### Visual Layers

Every emitted draw belongs to one semantic layer rather than being rediscovered through effect eligibility flags during each technique expansion:

1. `surface_opaque`: depth-writing opaque geometry that can produce the surface record and consume material AO.
2. `surface_masked`: alpha-tested or coverage-producing geometry with explicit surface-record participation.
3. `transparent`: blended, weighted, or peeled surface geometry that does not contribute to the primary opaque surface record unless a technique explicitly defines it.
4. `volume`: ray-marched or slice-based volume work with separate first-hit semantics.
5. `overlay`: annotations, UI-like visuals, gizmos, and screen-space content composed after scene effects unless a contract opts in.
6. `query`: picking/probe work that uses semantic geometry and identity but bypasses presentation effects.

Visual metadata declares layer membership and product capabilities once. Technique composition consumes those declarations. A visual must not branch on technique family names.

Visual order and technique phase order are separate. Visual order preserves authored source-over order, label/annotation order, and fixed-controller semantics within a layer. Technique phases order prepasses, opaque work, transparent algorithms, resolves, and postprocessing. The composer may not use a phase transition to silently reorder overlapping blended visuals.

### Technique Phases

The composer orders work through semantic phases:

1. `surface_capture`: produce or resolve the coherent surface record when required.
2. `surface_analysis`: derive products such as ambient visibility from the coherent surface record without producing scene color.
3. `opaque_shading`: produce the base scene color and consume material inputs such as ambient visibility.
4. `surface_postprocess`: transform opaque surface color using its coherent surface record; EDL runs here after AO-aware material shading and before transparency.
5. `transparent_shading`: accumulate and resolve transparent surface work against the declared scene color and depth products.
6. `volume_shading`: composite volume work using explicitly compatible scene or volume depth products.
7. `scene_postprocess`: transform the composed scene color or derive presentation effects such as DoF, bloom, outlines, or tone mapping.
8. `overlay`: compose content that should not receive scene-space lighting or depth effects.
9. `presentation`: encode or copy to the external panel target.
10. `query`: execute independent query plans outside presentation composition.

The phase order is a default partial order, not an effect-name list. A technique declares dependencies between product producers and consumers; the frame graph supplies the exact topological order and rejects cycles or multiple ambiguous producers.

A technique that transforms color consumes one `scene_color` product version and produces a distinct successor version. This versioning expresses the composition chain without multiple producers or read/write feedback on one product identity.

AO requires a coherent `surface_capture` prepass before visibility evaluation and forward `opaque_shading`, so eligible opaque and masked geometry is drawn twice when AO is enabled. Producing the record as an MRT of the same shading pass that consumes `ambient_visibility` would form a cycle and is forbidden. MRT piggyback remains valid for later consumers that do not feed the producing pass, such as EDL when AO is disabled, or after a separately approved deferred-lighting design.

## Technique Contract And Composer

A technique is an immutable internal descriptor with:

1. a stable typed technique ID and version;
2. its phase and ordering constraints;
3. required and optional input product signatures;
4. produced product signatures;
5. visual-layer participation requirements;
6. capability requirements and explicit adaptation choices;
7. a pass-expansion function that emits generic graph resources, passes, dependencies, bindings, pipelines, and draw filters;
8. a diagnostic name used only for logs and inspection, never identity or lookup.

The declarative work payload for an expanded pass includes the pipeline-provider key, work class such as visual draws, fullscreen draw, or compute dispatch, product-to-binding map, clear/load/store policy, sample and resolve policy, viewport and panel-local coordinate transform, ordered draw filter, and diagnostics. Generic lowering is not complete while runtime code must recover any of these facts from an effect name or resource suffix.

The panel composer performs one centralized transaction:

```text
panel visuals + retained technique state + target capabilities
    -> required product closure
    -> compatible producer selection
    -> capability and format resolution
    -> technique expansion
    -> graph validation and transient lifetime analysis
    -> immutable FramePlan
```

Composition must be deterministic for the same retained state and capability snapshot. It rejects incompatible product semantics, missing producers, multiple unresolved producers, phase cycles, illegal sample transitions, and consumers that read undefined background values.

Technique expansion is declarative. Runtime code lowers generic graph passes and resolved resource bindings; it must not allocate `SceneSsaoTargets`, `SceneEdlTargets`, `SceneWboitTargets`, or `SceneDepthPeelTargets` families or recover pass identity with `work_label` string comparisons.

## Color And Lighting Contract

All lighting and scene postprocessing operate on linear scene-referred color. Presentation encoding occurs exactly once in the presentation phase. Alpha representation is declared for every color product and conversion is explicit.

The material lighting function is decomposed conceptually into emissive, ambient or indirect diffuse, direct diffuse, and specular terms. `ambient_visibility` modulates only the ambient or indirect diffuse term by default:

```text
lit = emissive + ambient_visibility * indirect_diffuse + direct_diffuse + specular
```

This avoids the physically incorrect black overlay used by the current SSAO composite. A debug view may present the AO product directly, but ordinary composition must not darken direct, specular, emissive, overlay, or presentation-space contributions.

RC3 does not require an always-on HDR intermediate. The composer allocates a higher-precision `scene_color` only when material range or an enabled technique requires it; the concrete format is capability-resolved and recorded in diagnostics.

## Surface Capture And SSAO

When any enabled technique requires surface geometry, the composer schedules one shared surface-capture path for all eligible visuals. AO requires this prepass before `surface_analysis` and forward opaque shading, accepting duplicate eligible geometry for correctness. A compatible opaque-shading MRT may satisfy only later consumers that cannot feed the producing pass; optimization never overrides dependency acyclicity or portability.

Surface capture must describe the same nearest visible opaque or masked fragment as ordinary surface shading, including clip/discard rules, deformation, culling, depth bias, analytic sphere-impostor ray intersection, corrected fragment depth, alpha-to-coverage, and generated normals. Unsupported visual families are diagnosed or excluded explicitly; they never emit best-effort normals or depth. Transparent, volume, unlit overlay, and query layers are excluded from AO production and reception in the RC3 contract.

The RC3 ambient-visibility implementation should use a deterministic horizon-based view-space method in the GTAO family rather than the current sparse hit-count-normalized SSAO kernel. Required properties are:

1. occlusion normalization uses the complete declared direction/sample domain rather than only geometry hits;
2. radius, thickness, falloff, and bias are defined in view-space or another explicit physical scene scale;
3. background and rejected samples contribute unoccluded visibility rather than changing the denominator;
4. sampling is stable under stationary-camera redraws and does not use visible per-pixel random rotation without a matching reconstruction strategy;
5. the estimator is bounded before artistic intensity or contrast mapping;
6. normal, depth, and coverage validity are tested coherently at silhouettes and resolved edges;
7. projection reconstruction supports perspective and orthographic cameras through declared camera parameters;
8. reverse-Z or alternative hardware-depth choices cannot change the sampled linear-depth contract.

For a fixed camera, panel rectangle, render scale, technique state, backend, and resolution, AO uses no temporal jitter and produces deterministic graph structure and stable pixels. Cross-backend and cross-GPU conformance uses bounded numeric and perceptual tolerances rather than promising bit-identical floating-point images.

AO may run at full or panel-relative reduced resolution. A reduced-resolution product requires depth-aware and normal-aware reconstruction. Denoising uses separable or otherwise bounded edge-aware filtering whose world-space footprint is derived from the AO radius and projection, not a fixed unqualified pixel radius. The quality preset owns sample directions, direction steps, resolution scale, and denoise taps; ordinary users configure semantic appearance rather than kernel internals.

The proposed public SSAO descriptor contains semantic radius, intensity, thickness or bias, quality, optional minimum visibility, and debug mode. Existing `sample_count`, pixel blur radius, blur enable, and blur sigma controls should be removed or moved to an explicitly unstable expert/debug descriptor. Exact names and defaults are an approval decision, followed by public header, generated ctypes, examples, and documentation updates in one checkpoint.

## MSAA And Resolve Semantics

MSAA is orthogonal to techniques. The composer inserts a resolve only when a multisampled producer feeds a single-sampled consumer, and the product kind chooses the resolve:

| Product | Resolve rule |
| --- | --- |
| Linear scene color | Weighted or conventional color resolve in the same linear color space and alpha convention. |
| Hardware depth | Backend-supported depth resolve selected by the consumer's nearest-surface contract, or an explicit shader resolve when required. |
| Surface linear depth | Nearest valid covered surface under the declared depth convention, never an arithmetic average across foreground and background. |
| Surface normal | Normal belonging to the selected surface sample, or a coverage-weighted reconstruction followed by normalization when that exact policy is declared. |
| Surface coverage | Declared covered-sample fraction or binary winning-surface validity. |
| Object ID | Selected winning covered sample; never averaging, interpolation, or normalized filtering. |
| Ambient visibility | Normally evaluated after surface resolve; if multisampled AO is ever supported, its resolve must be declared separately. |

The graph rejects implicit sample-count changes and format/filter combinations that violate the product contract. Alpha-to-coverage sphere edges must supply coherent depth, normal, and coverage behavior rather than independently averaged attachments.

## Effect Migration

| Effect | RC3 relationship to products and phases |
| --- | --- |
| SSAO/GTAO | Produces `ambient_visibility` from the coherent surface record; opaque material shading consumes it. The old black-overlay composite disappears. |
| EDL | Consumes canonical linear `surface_depth` plus AO-aware opaque scene color and produces scene color in `surface_postprocess` before transparency; it no longer owns a private depth convention. |
| WBOIT | Keeps its accumulation algorithm but declares transparent accumulation, transmittance, scene-color, and depth products in `transparent_shading`. Transparent content is depth-tested against the opaque surface but does not produce or consume AO in RC3. |
| Depth peeling | Keeps peel iteration semantics but declares per-layer depth/color/transmittance products and explicit ordering in `transparent_shading`. Peeled content does not produce or consume AO in RC3. |
| Scene/volume occlusion | Produces or consumes typed `scene_occlusion_depth` with explicit contribution from compatible mesh or `volume_first_hit_depth` sources; `scene_occlusion_depth`, `surface_depth`, and `volume_first_hit_depth` remain distinct product kinds. |
| Depth cue | Remains material-local in RC3. A future panel fog technique may consume surface depth, but this refactor must not silently change depth-cue appearance. |
| DoF | Can later consume canonical surface depth and linear scene color without inventing another depth capture. It remains proposal-stage unless separately promoted. |
| Bloom/tone mapping | Can later consume linear scene color and produce presentation-ready color. RC3 establishes the encoding boundary without requiring either effect. |
| Outlines/selection | Can consume depth, normals, coverage, or IDs with explicit filtering and overlay composition. It remains deferred unless needed for conformance. |
| Picking/probe | Uses query products and semantic identities, bypassing AO, EDL, transparency presentation resolves, tone mapping, and overlays unless the query contract explicitly requests them. |

## Resource Scope And Lifetime

Technique products are panel-local by default. Extents are resolved from the panel render area rather than the full canvas, including offset, scale, rounding, minimum size, and resize invalidation. Source-relative resources inherit a validated source product extent.

Every panel-local product and pass carries the panel pixel origin, local extent, render scale, and local-to-target coordinate transform. Shaders sample local product coordinates rather than indexing a panel texture with global `gl_FragCoord`. Adjacent panels may not sample or alias active products across panel scope.

The frame plan computes first and last use for transient products. Runtime allocation may pool or alias products only when lifetimes do not overlap and format, extent, sample, access, and ownership requirements are compatible. Aliasing is an optimization; product identity and validation remain correct without it.

Persistent history products require an explicit owner, initialization rule, resize rule, invalidation events, and frame index. No RC3 technique in this proposal requires history.

## Capability Adaptation And Portability

Capability resolution occurs before graph expansion is finalized. Every adaptation is explicit in the resolved plan and diagnostics. Examples include concrete attachment formats, maximum sample count, depth resolve support, attachment count, filtering support, storage-texture availability, and compute availability.

Preferred and fallback implementations may differ in pass count or execution method but must preserve product semantics within declared tolerance. A fallback may reduce AO quality or resolution; it may not reinterpret normals, average object IDs, apply AO as a black overlay, or silently disable an enabled effect. Unsupported configurations fail with a precise capability diagnostic.

GLSL and WGSL shader interfaces should share generated or checked semantic declarations where feasible. WebGPU pressure-tests product formats, sampling rules, attachment limits, and resolve policies; it must not induce a second scene composer.

## Introspection And Validation

ASCII/JSON frame-plan output must expose product IDs, kinds, semantic metadata, producer and consumer pass IDs, resolved concrete formats, extents, sample counts, resolve policies, lifetimes, and capability adaptations. Human labels remain useful diagnostics but are never graph identity.

Required source and runtime invariants include:

1. no effect-family target structs in generic runtime lowering;
2. no pass or resource lookup by work-label string;
3. no visual shader or lowering branch keyed by technique family name;
4. exactly one unambiguous producer for each consumed product version;
5. coherent surface depth, normal, and coverage semantics;
6. no implicit color-space, alpha, sample-count, depth-convention, or normal-space conversion;
7. no sampled read before a producer and required transition;
8. no query identity filtering or averaging;
9. no postprocess mutation of borrowed presentation handles outside the granted contract;
10. deterministic graph output for identical state and capabilities.

Render conformance must include stable image or numeric evidence for AO under zoom, perspective and orthographic projection, silhouettes, isolated spheres, dense spheres, meshes, mixed eligible/ineligible visuals, MSAA off/on, alpha-to-coverage, resized and sub-panel rendering, disabled/fallback paths, and stationary redraws. Tests must distinguish algorithm correctness, shader ABI, graph structure, runtime execution, and presentation evidence.

## Migration Strategy

1. Introduce typed product metadata, graph validation, and introspection without changing existing effect output.
2. Introduce layers, phases, the composer, declarative work payloads, and generic resource/pass lowering behind parity tests. Effect-specific pipeline providers may remain until their contracts are representable, but effect-specific target families and string lookup paths are removed in the same migration window.
3. Establish the coherent surface record, including analytic sphere impostors, and semantic resolve policies.
4. Establish panel-local coordinates while retaining figure-sized physical allocations temporarily if needed for parity, then switch to panel-relative allocation in a separate checked checkpoint.
5. Replace SSAO evaluation and denoising, integrate ambient visibility into material lighting, and remove black-overlay composition.
6. Migrate EDL, WBOIT, and depth peeling to the shared products and phase contract.
7. Add capability fallbacks, binding/API updates, and the complete combination matrix.
8. Promote approved durable decisions into specialized scene specs and retire superseded prose before RC3 scope freezes.

Do not preserve both old and new composition architectures as long-lived paths. Short checkpoint-level compatibility scaffolding is acceptable only when removed before the final integration checkpoint.

## Maintainer Decision Gate

The recommended decisions are:

1. Approve typed products, semantic metadata, layers, phases, and the centralized composer as the v0.4 architecture.
2. Approve material-consumed ambient visibility and removal of the SSAO black-overlay composite.
3. Approve a deterministic GTAO-family estimator with edge-aware reconstruction and quality presets.
4. Approve removal of algorithm-specific SSAO knobs from the ordinary public descriptor, accepting API breakage.
5. Approve migration of EDL and both transparency techniques in this RC3 refactor rather than leaving effect-specific runtime paths behind.
6. Approve panel-relative transient products and product-specific MSAA resolve rules in the same architecture slice.
7. Approve opaque/masked-only AO participation for RC3: transparent, volume, unlit, overlay, and query work neither produces nor consumes AO.
8. Approve EDL after AO-aware opaque shading and before transparent/volume composition.
9. Approve analytic sphere impostors as a first-class surface-capture and AO conformance target.
10. Approve fixed-input deterministic AO on one backend and tolerance-based cross-GPU/backend visual conformance.
11. Defer temporal techniques, public plugins, editable frame graphs, a full deferred renderer, display HDR/color management, ray tracing, and broad new effects.

The maintainer approved all eleven recommended decisions without edits on 2026-08-02. Any implementation discovery that changes product semantics, scene/DRP2 ownership, the public API direction, or the migration scope returns to maintainer review rather than being decided by an execution subagent.

During R0 consistency review, the maintainer additionally approved the required AO prepass and typed `scene_occlusion_depth` refinement: AO-enabled panels capture the coherent surface record before `surface_analysis` and redraw eligible geometry for forward opaque shading, while scene-occlusion depth remains semantically distinct from surface and volume first-hit depth.
