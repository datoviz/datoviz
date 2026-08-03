# Reactive Application State

Status: exploratory v0.5+ cross-cutting architecture proposal; not an RC3, RC4, or final v0.4 release gate. Baseline reviewed: `73dc1ae0128b9897806fad470bd05c1143d7a316`. Updated: 2026-08-03.

## Decision Addressed

Datoviz applications need a coherent way to connect application-owned state, external GUI controls, direct scene interaction, generated Python bindings, and asynchronous results without duplicating variables, change flags, propagation callbacks, or worker-thread handoff code.

This proposal defines the smallest plausible native state and synchronization layer. It does not approve a public API, commit v0.5 scope, or make every interactive value a Datoviz property.

## Proposed Direction

Evaluate an app-scoped `DvzReactive` subsystem with four core concepts:

| Concept | Responsibility |
| --- | --- |
| property | Own one typed application value and its optional presentation metadata. |
| transaction | Commit an atomic, ordered set of property mutations with origin, edit phase, and generation. |
| endpoint | Describe typed access to a property or an existing Datoviz-owned value without transferring ownership. |
| binding | Propagate committed changes between compatible endpoints according to an explicit policy. |

Dear ImGui remains an immediate-mode external UI. Dense visual data remains in retained resource APIs such as `dvz_visual_set_data()` and its range variants. Scene and GPU mutations remain on the owning application thread.

## Single-Authority Rule

Every interactive concept has exactly one authoritative retained owner. A property is not automatically authoritative merely because it is observable, serializable, or displayed by a widget.

Application-owned concepts should use properties as their authoritative storage. Examples include analysis thresholds, acquisition parameters, display modes, workflow choices, and application-defined coordination values. GUI, Python, commands, and direct application interaction submit mutations to that property; bindings derive Datoviz target state from committed values.

Scene-owned concepts remain authoritative in their existing scene objects. Camera pose, panzoom domain, selection membership, hover state, panel transforms, and other values with scene invariants must continue to be read and changed through the owning controller or scene API. A GUI inspector may project an endpoint and invoke its setter, but it must not create a competing property copy.

The resulting ownership patterns are:

```text
application-owned concept
GUI / Python / application gesture -> property -> transaction -> scene endpoint

scene-owned concept
GUI / input gesture -> scene or controller endpoint -> canonical scene state
```

If an application deliberately promotes a scene-editable concept into application-owned state, every producer of that concept must route through the property. The controller must not independently retain and mutate a peer value.

## Properties

A property owns one small typed value. The first prototype should consider booleans, signed and unsigned integers, floats, doubles, enums, colors, and small vectors; strings, paths, arrays, and arbitrary blobs are deferred.

Stable property names may support lookup, persistence, inspection, Python access, diagnostics, and remote application control. Suggested names use application-defined dotted paths such as `analysis.threshold`, `display.point_size_factor`, or `acquisition.block_size`.

Optional presentation metadata may include a label, description, unit, format, default, range, step, enum labels, equality tolerance, and persistence policy. The runtime owns or interns copied metadata strings.

Properties must not wrap arbitrary caller pointers in the first slice. Owned storage gives the runtime a defined lifetime, type, generation, equality policy, and thread-affinity contract.

## Transactions

Transactions make propagation atomic and deterministic. A committed change records the property, old and new values, edit phase, origin, property generation, and commit generation.

The edit phases should distinguish an interactive edit beginning, updating, and ending from an atomic programmatic set. This permits cheap presentation updates during dragging while an application schedules expensive work only at edit end.

Repeated writes to one property in a transaction normally coalesce to the first old value and final new value. Binding application happens after validation and coalescing, never recursively inside a setter.

The first public candidate must settle explicit transaction ownership, implicit single-set transactions, nested transaction behavior, result lifetime, and whether completed transactions are borrowed snapshots or retained objects before headers are drafted.

## Endpoints

An endpoint is typed access to existing state, not another stored value. It records value type, readability, writability, equality behavior, target lifetime, thread affinity, and any capability constraints.

Public endpoint construction must not expose an untyped `void*` plus an arbitrary string as the stable ABI. String paths may be useful for application lookup and introspection, but they should resolve once through a typed provider into an opaque endpoint or a generator-friendly object-kind and field descriptor.

Endpoint paths or field identifiers become supported schema once public. Unknown fields, type mismatches, unsupported capabilities, destroyed targets, and thread violations must fail before rendering and produce inspectable diagnostics.

The initial endpoint registry should be deliberately small and justified by examples. Plausible pressure tests are an application property, a visual presentation modifier, selection or hover presentation, and a panzoom domain inspector.

Dense attributes such as per-item positions, colors, sizes, or voxel values are not endpoints in this model. They remain explicit retained data resources with range-aware update APIs.

## Bindings

One-way bindings are the preferred first slice. They propagate committed source changes to compatible targets continuously, on edit end, or through explicit application.

Unrestricted symmetric binding between two retained values is deferred. Generation tagging and equality suppression prevent recursion, but they do not decide semantic ownership or resolve concurrent edits.

A future bidirectional adapter is acceptable only when one endpoint is designated canonical and the other is a projection, cache, or editing surface with defined conflict and effective-value rules. Controller and multi-view prototypes must establish those rules before a generic public API is approved.

The first implementation should avoid a general dependency graph, arbitrary expression evaluation, user conversion callbacks, and implicit type coercion. Cycles among property bindings must fail during construction or be disabled with a diagnostic.

## GUI Projection

Metadata-driven Dear ImGui controls are a useful projection of properties and readable/writable endpoints. Widget selection may use type, range, enum, label, description, unit, step, and format metadata.

Raw cimgui and the existing `dvz_gui_*` wrappers remain supported escape hatches. A reactive GUI helper is additive and must remain in the optional GUI integration surface rather than making the generic reactive core depend on cimgui or C++.

A property widget edits application-owned storage. A scene inspector edits the scene endpoint directly or through its canonical setter. The API should preserve that distinction even if both controls share internal rendering helpers.

## Async And Commands Boundary

Native commands and a managed task executor are possible later extensions, not requirements of the first reactive slice.

Background work must follow the established snapshot/work/apply contract. Workers consume owned immutable input and never access live scene, runtime, DRP2, GUI, or GPU objects. Results return to the owner thread through the existing `dvz_view_post()` and wake mechanism, then enter application state through an ordinary transaction if appropriate.

Generation-tagged stale-result rejection remains desirable application infrastructure, but it does not require `DvzReactive` to own a worker pool. A bounded native executor should be proposed separately only after C applications demonstrate repeated need beyond the implemented post primitive and host-native executors.

Python may continue to use the existing host, `asyncio`, thread-pool, and process-pool integration. The generated binding should expose the C-shaped reactive API honestly; a higher-level Python property façade is informative pressure, not part of this proposal's support commitment.

## Module And Header Boundary

If prototypes demonstrate a reusable non-scene consumer, the generic property, transaction, endpoint, and binding core may become a scene-independent `reactive` module. It must be testable without a scene, Vulkan, GUI, cimgui, or Python runtime.

Scene endpoint providers and invalidation adapters remain scene-owned. GUI projection remains GUI-owned. App integration owns scheduling and the association between a reactive runtime and application-thread lifetime.

Candidate source boundaries are:

```text
src/reactive/          generic values, properties, transactions, and bindings
src/scene/reactive/    scene/controller endpoint providers and invalidation adapters
src/gui/reactive/      optional Dear ImGui projection
```

Public header placement, umbrella-header inclusion, and whether bindings need a separate header should follow a working prototype rather than the original RFC's provisional filenames.

## Relationship To Existing Authority

This proposal must remain consistent with:

1. [`MODULE_LAYERS.md`](MODULE_LAYERS.md) for reusable subsystem promotion.
2. [`../api/PUBLIC_API_CONVENTIONS.md`](../api/PUBLIC_API_CONVENTIONS.md) for names, records, ownership, results, and generated-binding shapes.
3. [`../api/PYTHON_GSP_SCOPE.md`](../api/PYTHON_GSP_SCOPE.md) for the Datoviz, generated Python binding, GSP, and VisPy2 boundary.
4. [`../bindings/README.md`](../bindings/README.md) for the generated `ctypes` architecture.
5. [`../bindings/ASYNC_CALLBACKS.md`](../bindings/ASYNC_CALLBACKS.md) for implemented owner-thread dispatch and Python event-loop integration.
6. [`../scene/integration/EXTERNAL_UI.md`](../scene/integration/EXTERNAL_UI.md) for external UI and scene-state ownership.
7. [`../scene/integration/THREAD_SAFETY.md`](../scene/integration/THREAD_SAFETY.md) for snapshot and thread-affinity rules.
8. [`../scene/pipeline/INVALIDATION_AND_CACHING.md`](../scene/pipeline/INVALIDATION_AND_CACHING.md) for scene mutation consequences.
9. [`../scene/proposals/future/VISUAL_MODIFIERS.md`](../scene/proposals/future/VISUAL_MODIFIERS.md) for scene presentation targets.

## Prototype Sequence

The proposal should advance only through narrow, independently reviewable prototypes:

1. Private typed property storage, metadata ownership, transactions, equality, origins, and edit phases with CPU-only tests.
2. One-way bindings between properties with deterministic ordering, lifetime safety, cycle rejection, and no steady-state allocation.
3. Optional GUI projection proving immediate-mode editing and transaction phases without scene dependencies.
4. One scene endpoint proving canonical ownership, validation, invalidation, destruction safety, and generated Python binding behavior.
5. A controller inspector proving direct endpoint projection without a duplicate property.
6. A coordinated application-owned property proving one property can drive several scene targets and remain visible to GUI and Python.

Only after those prototypes should maintainers decide whether the concepts become supported public API, remain internal application helpers, or move above Datoviz.

## Acceptance Pressure Test

A successful prototype should let an application define an authoritative `display.selected_size_factor` property, edit it through Dear ImGui or Python, propagate it to a supported selection presentation target without uploading dense size data, inspect the committed transaction, and schedule optional expensive work on edit end while keeping all scene mutations on the owner thread.

A separate controller inspector should display and edit panzoom state through the canonical controller endpoint without retaining a second domain value.

## Non-Goals

The initial architecture is not a retained widget tree, virtual DOM, functional reactive programming framework, arbitrary C reflection system, expression language, dependency-injection container, scientific workflow engine, universal shader reflection layer, dense-data replacement, or promise of thread-safe scene mutation.

It also does not recreate high-level Python plotting or application workflow APIs that belong to GSP or VisPy2.

## Open Decisions

1. Exact value representation and ABI shape for generated bindings.
2. Transaction handle, lifetime, polling, and nesting semantics.
3. Opaque endpoint versus typed public descriptor representation.
4. Endpoint discovery and versioning policy.
5. Read-only or derived endpoint support in the first prototype.
6. Effective-value reporting when a target validates, clamps, or normalizes a request.
7. Whether a reactive runtime is owned by, attached to, or merely scheduled through `DvzApp`.
8. The minimum evidence required before reconsidering canonical bidirectional adapters, commands, persistence, or a native task executor.
