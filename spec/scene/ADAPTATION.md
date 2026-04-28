# Scene Capability Adaptation

This document defines how the future scene layer should adapt to runtime capability differences.

Capability adaptation is a scene-side policy step.

It decides how validated scene intent changes when the runtime contract cannot provide the preferred
path.


## Purpose

Capability adaptation should:

1. keep the scene layer backend-agnostic,
2. make fallback behavior explicit and deterministic,
3. avoid silent degradation that changes scene meaning unexpectedly,
4. keep native and browser paths on the same semantic policy surface,
5. give planning a clear post-adaptation input state.


## Position

Capability adaptation sits between:

1. scene validation,
2. runtime capability query,
3. `FramePlan` construction,
4. DRP2 emission.

The intended order is:

1. scene objects define preferred semantic behavior,
2. the scene validates that preferred behavior,
3. runtime capability information is applied,
4. the scene chooses one explicit adaptation outcome,
5. planning proceeds from the adapted scene intent.


## Core Rule

Capability adaptation must preserve semantic clarity, not merely visual resemblance.

If the preferred path is unavailable, the scene may only:

1. reject the configuration,
2. simplify it explicitly,
3. deactivate the affected feature or object explicitly.

The scene should not:

1. silently reinterpret scene meaning,
2. leak backend-specific toggles into public scene semantics,
3. rely on backend best-effort behavior to define fallback,
4. invent per-backend scene behavior that cannot be explained at the scene level.


## Non-Goals

This document does not define:

1. the exact public API for capability negotiation,
2. the final runtime capability struct shape,
3. backend-specific implementation strategy,
4. benchmarking or quality scoring policy,
5. user-interface policy for how diagnostics are displayed.


## Capability Input Model

The scene layer should consume a declarative runtime capability report.

That report should come from the runtime boundary, not from backend internals.

At minimum, adaptation may depend on capabilities such as:

1. compute support,
2. readback support,
3. offscreen target support,
4. supported texture formats,
5. supported sample counts,
6. relevant resource limits,
7. shader language support,
8. FP64 availability,
9. determinism-related guarantees,
10. color blending and render-target attachment limits,
11. optional picking support if modeled separately.

This aligns with the DRP2 capability model in
[spec/drp2/CAPABILITIES.md](/home/cyrille/GIT/Viz/datoviz/spec/drp2/CAPABILITIES.md).


## Adaptation Outcomes

The scene spec should restrict capability adaptation to three explicit outcomes:

1. accept,
2. simplify,
3. deactivate or reject.


### Accept

The preferred scene intent is fully supported.

No semantic change is needed.


### Simplify

The preferred scene intent is adjusted to a reduced but still semantically acceptable form.

Examples:

1. use a lower-fidelity annotation rendering path,
2. drop optional interactivity while keeping explanatory content,
3. choose a simpler visual variant that preserves the same data meaning,
4. replace a compute-assisted acceleration path with a non-compute path when semantics remain intact.


### Deactivate Or Reject

If no semantically acceptable simplification exists, the scene must either:

1. deactivate the affected object or feature with a scene-visible diagnostic, or
2. reject the configuration as unsupported.

Which of these is allowed depends on the scene contract for that object.


## Semantic Preservation Rule

Not every visual or feature may be simplified.

The adaptation policy should distinguish between:

1. semantic requirements,
2. quality preferences,
3. optional affordances.


### Semantic Requirements

If adaptation would change what the scene means, the scene should reject or deactivate rather than
pretend it still supports the requested result.

Examples:

1. a colorbar for a scalar mapping cannot be replaced by an unrelated categorical legend,
2. a picking-dependent workflow cannot claim picking support if identity round-trip is impossible,
3. a volume visual that fundamentally requires 3D sampled data cannot be silently shown as a 2D
   image with different semantics.


### Quality Preferences

If adaptation changes fidelity but not meaning, simplification may be acceptable.

Examples:

1. reduced annotation decoration,
2. lower sampling precision where the scene contract allows it,
3. simpler sample marks in a legend,
4. lower-fidelity offscreen export path if deterministic semantics remain intact.
5. lower-fidelity transparency realization when the same visibility and emphasis semantics remain
   readable.


### Optional Affordances

If a feature is explicitly optional, deactivation may be acceptable.

Examples:

1. disabling interactive legend entry picking,
2. omitting decorative overlays,
3. disabling timestamp-query-backed diagnostics,
4. suppressing optional hover probes.


## Adaptation Scope

Capability adaptation should run on the smallest correct scope.

Useful scopes include:

1. one visual,
2. one annotation,
3. one legend or colorbar,
4. one panel target path,
5. one export request,
6. one full scene build.

This avoids unnecessary degradation of unrelated objects.


## Adaptation Order

The preferred order is:

1. validate preferred semantics,
2. classify requirements versus preferences,
3. consult runtime capabilities,
4. choose one explicit adaptation outcome,
5. record diagnostics and invalidation consequences,
6. plan from the adapted state.

The scene should not build a `FramePlan` first and only afterward discover that the plan depended on
unsupported capabilities.


## Adaptation Precedence

When several adaptations are possible, the scene should prefer:

1. preserving semantic meaning,
2. preserving object visibility,
3. preserving interaction only when it does not alter meaning,
4. preserving higher fidelity last.

In practice:

1. keep the object if meaning can be preserved,
2. preserve semantic emphasis relationships such as selected-versus-context visibility before
   preserving the highest transparency quality,
3. treat high-fidelity transparency algorithms as quality choices unless one is explicitly required by
   the scene contract.

Weighted blended OIT is an adaptation outcome derived from lower-level capabilities, not a single
runtime capability by itself. The scene should select it when the runtime supports the required
floating-point render targets, blending behavior, color-attachment count, and pass structure; when
those ingredients are absent, the scene should choose a declared fallback or emit a diagnostic.


## Transparency And Emphasis

Transparency should be treated as a scene-visible styling and emphasis policy, not as a backend
algorithm choice.

The scene should therefore distinguish between:

1. semantic intent such as context meshes shown translucently,
2. semantic emphasis such as selected objects shown more opaquely than unselected context,
3. backend realization choices such as which transparency path the runtime uses.

The preferred adaptation rule is:

1. preserve the semantic relationship first,
2. simplify the realization quality second,
3. reject only when the requested semantics themselves cannot be represented honestly.

For examples such as anatomical context surfaces plus selected region emphasis:

1. the scene may request one outer mesh as translucent context,
2. the scene may request interior regions as translucent by default,
3. the scene may request the selected region to become more opaque or otherwise more prominent,
4. the runtime may realize that request with different transparency-quality paths depending on
   capability,
5. but adaptation should not silently erase the difference between selected and unselected state if
   that difference is semantically important to the workflow.

Acceptable simplifications may include:

1. lower-quality transparency while preserving ordering semantics well enough for the declared use,
2. reduced layering fidelity while keeping selected-versus-context emphasis visible,
3. disabling nonessential translucent decorative overlays before degrading primary scientific
   objects.

Unacceptable simplifications include:

1. silently making all context and selected objects equally opaque when the scene relies on that
   difference for interpretation,
2. silently making all translucent context invisible if visibility of those objects is still part of
   the intended scene meaning,
3. claiming high-quality transparency semantics when the active capability path only supports a
   materially different result and no explicit adaptation diagnostic was produced.
2. reduce quality before removing the object,
3. remove optional affordances before changing data interpretation,
4. reject rather than silently change scientific meaning.


## Relationship To Validation

Capability adaptation depends on the validation rules in
[VALIDATION.md](VALIDATION.md).

The distinction should remain clear:

1. validation asks whether the preferred scene configuration is coherent,
2. adaptation asks whether the runtime can realize that coherent configuration,
3. validation failure means the scene is wrong,
4. adaptation failure means the runtime capability set is insufficient for the requested semantics.


## Relationship To Runtime Boundary

Capability adaptation must respect the runtime boundary in
[RUNTIME_BOUNDARY.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/RUNTIME_BOUNDARY.md).

This means the scene may adapt based on:

1. declarative capability fields,
2. DRP2-visible feature support,
3. readback and offscreen services,
4. resource-limit reports.

It must not adapt based on:

1. Vulkan constants in public scene logic,
2. backend-private handle types,
3. backend memory-allocation details,
4. private command-buffer implementation behavior.


## Capability Classes That Matter To Scene

The most important scene-facing capability pressures are:

1. compute availability,
2. readback and picking support,
3. offscreen and export support,
4. format and size limits,
5. FP64 support,
6. determinism guarantees,
7. optional native-only ingestion paths kept outside core scene semantics,
8. hardware ray tracing (future).

### Hardware Ray Tracing (Future Capability Class)

Hardware ray tracing (Vulkan Ray Tracing extension or equivalent) is a future capability class.

When present and requested, the scene emits `RayTraceNode` instead of `RenderNode` for
participating visuals.
When absent or not requested, the scene falls back to `RenderNode` rasterization.

The fallback policy follows the standard adaptation model:

1. if ray tracing is declared **required** and unavailable, validation fails,
2. if ray tracing is declared **preferred**, the scene silently falls back to rasterization,
3. if ray tracing is declared **off** (default), rasterization is always used.

The scene API and light source model are identical in both paths.
See `LIGHTING.md` for the full forward-compatibility design.


## Compute Availability

Compute may be mandatory at the DRP2 contract level, but individual scene features may still need an
adaptation policy.

Examples:

1. a scene feature uses compute only as an acceleration path,
2. a visual variant depends on compute for one optional derived effect,
3. a readback-oriented analysis path depends on compute-assisted preprocessing.

Allowed adaptation patterns:

1. replace compute acceleration with a non-compute path if the meaning is preserved,
2. disable only the optional effect if it is not semantically required,
3. reject the feature if compute is semantically essential.


## Readback And Picking

Picking and offscreen workflows require explicit capability-aware handling.

Allowed adaptation patterns:

1. disable optional hover picking while preserving display,
2. keep click selection only if identity round-trip remains valid,
3. reject picking-dependent tools when readback or routing requirements are unavailable,
4. keep a visual visible while deactivating optional interaction overlays.

The scene must not claim picking semantics when stable identity round-trip is unavailable.


## Offscreen And Export

Export-oriented scenes need explicit adaptation policy.

Allowed adaptation patterns:

1. use a reduced annotation set in constrained export targets,
2. disable optional interactive overlays in headless output,
3. reject export requests that require unavailable deterministic readback or target modes.

The scene should preserve reproducibility rules when the contract requires deterministic export.


## Format And Limit Constraints

Some adaptations are driven by supported formats, dimensions, or size limits.

Allowed adaptation patterns:

1. choose a lower-cost internal scene variant that still preserves meaning,
2. reduce optional sample count or decorative target usage,
3. split one large derived artifact into smaller logical work only if the scene contract allows it,
4. reject requests whose scientific meaning depends on unsupported formats or limits.

The scene should not silently rescale or truncate user data in a way that changes meaning.


## FP64 And Precision

FP64 is explicitly capability-gated in the DRP2 model.

Scene adaptation should therefore classify precision needs explicitly.

Useful classes are:

1. FP64 required for semantic correctness,
2. FP64 preferred for quality or robustness,
3. FP64 irrelevant.

Allowed adaptation patterns:

1. choose an FP32 path only when the scene contract marks FP64 as optional,
2. emit a warning or diagnostic when precision is reduced,
3. reject the configuration when the requested workflow requires FP64 for correctness.


## Determinism

Determinism should be treated as a first-class capability input.

Useful scene-side policies include:

1. best effort,
2. deterministic required,
3. deterministic preferred.

Allowed adaptation patterns:

1. continue on a best-effort path only when deterministic behavior was not required,
2. warn when deterministic preference cannot be satisfied,
3. reject export, testing, or scientific replay workflows when determinism is required and not
   guaranteed.


## Visual-Family Adaptation Pressure

Each visual family should eventually declare which capability losses can be simplified and which
force rejection.

Examples:

1. `point` may allow simpler variants or reduced decorative effects,
2. `image` may allow some display simplifications but not loss of the underlying sampled-field
   meaning,
3. `volume` may have fewer acceptable simplifications because its semantics are more tightly coupled
   to 3D sampled data and traversal behavior,
4. `glyph` and annotation-heavy paths may allow display simplification while preserving text meaning.


## Annotation Adaptation Pressure

Annotations often allow more simplification than primary data visuals.

Allowed adaptation patterns may include:

1. removing decorative backgrounds,
2. reducing guide complexity,
3. disabling pickable annotation behavior,
4. collapsing dense overlays into simpler summaries,
5. omitting optional probes.

But the scene should still reject adaptations that erase required explanatory meaning.


## Legend And Colorbar Adaptation Pressure

Legends and colorbars should define their own adaptation priorities.

Preferred order:

1. preserve the explanatory mapping,
2. preserve labels and tick semantics,
3. preserve placement,
4. preserve decoration and interactivity last.

Examples:

1. simplify sample marks before dropping entry labels,
2. reduce ramp fidelity before dropping the colorbar entirely,
3. disable pickable legend entries before removing the legend,
4. reject the colorbar if the required scalar mapping itself cannot be represented honestly.


## Panel And Multi-Panel Adaptation

Adaptation may differ across panels.

The scene should allow:

1. one panel to disable optional interaction while another keeps it,
2. export panels to use stricter deterministic policy,
3. consolidated legends only where mappings are genuinely shared,
4. panel-local deactivation without degrading the whole scene.

This again argues for small-scope adaptation rather than global fallback whenever possible.


## Diagnostics

Capability adaptation should always produce scene-visible diagnostics when the preferred path was not
taken.

A useful conceptual diagnostic should report:

1. affected object identity,
2. required capability,
3. chosen adaptation outcome,
4. semantic impact,
5. whether the result is a warning, deactivation, or hard failure.

In terms of `DIAGNOSTICS.md`:

- **phase**: `CapabilityAdaptation`
- **category**: `Capability`
- **subject_kind**: typically `Visual`, `Annotation`, `Legend`, or `Panel`
- **severity**: `Warning` for simplification; `Recoverable` for deactivation; `Fatal` for
  hard rejection that prevents planning from proceeding.

When a visual is deactivated (outcome = `deactivate`), the scene emits a `Recoverable` diagnostic
with `subject_kind = Visual` and a message explaining which capability was missing. No DRP2 commands
are emitted for that visual in the affected frame.


## Caching And Invalidation Consequences

Capability adaptation may change what must be rebuilt.

Examples:

1. selecting a simpler variant may invalidate `FramePlan`,
2. disabling picking may invalidate readback routing,
3. changing export policy may invalidate panel-local annotation layout,
4. selecting a reduced precision path may invalidate derived resources.

Adaptation policy should therefore integrate with
[pipeline/INVALIDATION_AND_CACHING.md](pipeline/INVALIDATION_AND_CACHING.md).


## Worked Adaptation Examples

Useful canonical examples include:

1. a hover probe deactivates because picking readback is unavailable,
2. a legend remains visible but loses interactive entry toggling,
3. a visual chooses an FP32 variant with a warning because FP64 was preferred but not required,
4. an export request is rejected because deterministic offscreen readback is required but
   unsupported,
5. a compute-accelerated derived path falls back to a non-compute path with unchanged semantics.


## What This Document Intentionally Leaves Open

This document intentionally does not freeze:

1. the exact adaptation-policy API surface,
2. whether policy is declared per object, per family, or via scene-wide defaults,
3. the final diagnostic severity taxonomy,
4. the exact mapping from runtime capability fields to scene policy hooks,
5. whether some adaptation choices are user-configurable at runtime.


## Immediate Follow-Up

With validation and capability adaptation specified, high-value follow-on work includes more
worked examples, especially multi-panel and annotation-heavy cases.
