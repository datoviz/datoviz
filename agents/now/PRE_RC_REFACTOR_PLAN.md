# Pre-RC Architecture Refactor Plan

Status: approved maintainer handoff for autonomous execution.
Updated: 2026-07-01.

This plan records the manual architecture decisions required before an agent performs broad
pre-RC refactoring. A future high-reasoning Codex agent may execute this document end to end with
checkpoint commits, without asking for routine implementation choices.

This is a replacement/refactor plan, not a compatibility layering plan. An executing agent must not
leave old and new authorities active side by side. If a better system replaces an existing one,
remove, generate, or explicitly de-authorize the old path in the same phase commit.


## Mission

Make the v0.4 pre-RC architecture more future-proof while API/ABI compatibility can still be
broken. Focus on boundaries that would be expensive to change after RC1:

1. Public API tiering.
2. Backend-neutral rendering protocol types.
3. DRP2 command metadata authority.
4. WASM scene bridge structure and generated registries.
5. Panel render planning.
6. Optional scene internal state cleanup if the required phases pass cleanly.

Do not preserve v0.3 compatibility at the expense of v0.4 architecture, correctness, or
maintainability.


## Global Invariants

1. The active runtime path remains:

   ```text
   scene frame plans -> drp2 command streams -> vklite runtime ->
   canvas/stream frame execution -> optional app presentation
   ```

2. Do not create parallel renderers, presentation layers, frame streams, or Vulkan wrappers.
3. Scene public APIs and DRP2 protocol APIs must be backend-neutral.
4. Vulkan, vklite, stream, and low-level runtime APIs are opt-in advanced surfaces unless a phase
   explicitly promotes a symbol.
5. `datoviz.raw` may remain broad and exact, but top-level Python exposure must be intentional.
6. Generated C/JS metadata files should be committed when normal builds, docs, tests, or browser
   routes depend on them.
7. Update examples, docs, manifests, and tests in the same phase commit when an intentional API or
   advanced-surface change requires it.
8. Keep generated/runtime binary payloads out of commits unless the user explicitly approves those
   exact files in the current turn.
9. Never stage or commit the `data` submodule pointer unless the user explicitly approves that
   submodule commit or pointer update in the current turn.
10. Treat staged `data` gitlink updates as stop signs.
11. Ignore unstaged/untracked `data` working-tree state only if it remains unstaged and uncommitted.
12. Every metadata-backed boundary must have exactly one authority. Do not add a second manifest,
    enum table, route list, switch, or generated file unless it is generated from, or mechanically
    validated against, the existing authority.
13. Prefer aggressive replacement over mixed legacy/new coexistence. Temporary compatibility code is
    allowed only when it is trivial, explicitly deprecated, and protected by drift checks that keep
    the new authority dominant.


## Stop Conditions

The executing agent should proceed autonomously except in these cases:

1. A public API name has two plausible long-term choices not decided in this document.
2. Tests fail for reasons that appear unrelated to the current phase after investigation.
3. The staged set contains `data`, generated/runtime binaries, vendored runtime libraries, large
   binaries, or unrelated user changes.
4. The implementation must widen beyond the phase scope to remain correct.
5. A required code-generation path would make normal builds depend on unavailable tools.
6. The only clean implementation would leave two active authorities or two runtime paths for the
   same concept.

If a stop condition occurs, record the exact blocker, current diff state, and preferred next move.


## Baseline Execution Rules

Before Phase 1, perform a baseline pass and record the result in the phase notes:

```sh
git status --short
git submodule status data
git diff --check
just build
just spec-check
```

If baseline validation fails before any edits, classify the failure as either an existing blocker or
a known environmental limitation before continuing. Do not hide baseline failures inside a later
phase.

During implementation, search for existing authorities before creating a new file. The current repo
already has these likely authorities:

1. DRP2 prose and schema: `spec/drp2/COMMANDS.md`, `spec/drp2/schema/README.md`,
   `spec/drp2/schema/drp_command.json`, and `spec/drp2/schema/commands/*.json`.
2. Example and browser-route metadata: `examples/c/MANIFEST.yaml`, checked by
   `tools/check_example_manifests.py`.
3. Public API policy: `spec/api/`, generated docs inputs, and installed headers under
   `include/datoviz/`.

Create a new authority only after proving the existing one cannot represent the required data. If a
new authority is created, migrate consumers to it and remove or validate the old source in the same
phase.


## Checkpoint Commit Rules

Make one logical checkpoint commit per completed phase. Before each commit:

1. Run the narrow validation listed for the phase.
2. Run `git diff --check`.
3. Run `git status --short`.
4. Stage only the phase's intended files.
5. Run `git diff --cached --stat`.
6. Verify the staged set excludes unapproved `data`, generated/runtime binaries, vendored runtime
   libraries, large binaries, and unrelated user changes.

Update this file after each phase with the commit hash, validation commands, failures if any, and
known follow-up risk.


## Approved Manual Decisions

### API Tiering

Create a machine-readable API status manifest:

```text
spec/api/status.yml
```

Use these tiers:

1. `stable`: default public C/Python surface.
2. `experimental`: public enough to try, explicitly allowed to change.
3. `advanced`: opt-in low-level/runtime API.
4. `internal`: not a supported user API.

Minimum manifest fields:

1. `module`: stable module key used by docs and generators.
2. `headers`: installed headers covered by the entry.
3. `tier`: one of `stable`, `experimental`, `advanced`, or `internal`.
4. `umbrella`: whether the header is included by the default stable umbrella, advanced umbrella, or
   neither.
5. `raw_binding`: whether `datoviz.raw` should expose the symbols exactly.
6. `python_top_level`: whether top-level `datoviz` should expose/adapt the symbols.
7. `docs_group`: generated docs grouping or `null`.
8. `rationale`: short reason for non-stable tiering.

Use symbol-level overrides only when a header-level classification would create a misleading API
boundary. Do not create a second tiering table in docs, bindings, or tests; consumers must read or
validate against `spec/api/status.yml`.

`include/datoviz/datoviz.h` should become the stable/default umbrella. Low-level DRP2, vklite, vk,
window, stream, and runtime APIs should be opt-in through explicit headers, with an
`include/datoviz/advanced.h` convenience umbrella if it simplifies migration. If `advanced.h` is
added, remove advanced transitive exposure from the stable umbrella instead of keeping both paths as
equivalent defaults.

Do not rely on prose-only "advanced/unstable" comments as the only API boundary. The manifest
should guide headers, generated docs, raw-binding policy, top-level Python facade policy, and symbol
export review where practical.

Stable scene/app examples should remain conceptually equivalent after this phase. Low-level DRP2,
vk, vklite, stream, WASM, raw-ctypes, and advanced examples may be updated, moved under an advanced
classification, or reclassified when their current shape depends on accidental public API.

### Backend-Neutral Rendering Types

Move public rendering protocol types out of Vulkan-facing headers. Prefer:

```text
include/datoviz/render_types.h
```

This header should own public types such as `DvzFormat`, primitive topology, compare op, cull mode,
front face, blend factors, blend ops, or closely related protocol enums needed by scene and DRP2.
Vulkan and WebGPU mappings must remain internal to their backend/runtime layers.

Move existing public protocol enums into `render_types.h`; do not duplicate enum definitions through
aliases or wrapper enums. `include/datoviz/vk/enums.h` may include `render_types.h` for backend
mapping, but it must not continue to own backend-neutral public protocol types. The existing
`include/datoviz/gpu/enums.h` primitive-topology ownership should either move into
`render_types.h` or become a compatibility forwarding header with no independent authority.

The following public names are pre-approved and should not trigger a naming stop condition:

1. `DvzFormat`
2. `DvzPrimitiveTopology`
3. `DvzCompareOp`
4. `DvzCullMode`
5. `DvzFrontFace`
6. `DvzBlendFactor`
7. `DvzBlendOp`

Preserve existing numeric values for moved enums where practical and low-risk, especially for
`DvzFormat`, to reduce transition churn and avoid unnecessary fixture/binding diffs. This numeric
preservation is incidental compatibility, not a public contract. Do not document Vulkan numeric
identity as part of the Datoviz API, and do not design new APIs that depend on it.

Update examples, generated bindings, documentation, and smoke fixtures in the same phase when
include paths, enum locations, or advanced API classifications change. Do not leave examples
silently compiling through obsolete Vulkan-facing includes when the correct dependency is
`include/datoviz/render_types.h` or the stable umbrella.

### Compatibility Stance

Source/API breaks are allowed now when they remove accidental backend leakage, accidental exports,
or bad ownership boundaries. Do not add ABI compatibility aliases. Add temporary source aliases only
when they are trivial, clearly deprecated, and do not pollute the new architecture.

When replacing an old public include or metadata path, do not leave examples, docs, or tests
silently exercising the old path. Either migrate them in the same commit or mark the old path as
advanced/internal and add a drift check proving it is no longer authoritative.

### DRP2 Command Authority

Use the existing DRP2 schema tree as the first candidate authority:

```text
spec/drp2/schema/
```

Generate or table-drive:

1. Command names.
2. Command kind/category.
3. Fixed body size.
4. Packet metadata.
5. JS constants/metadata.
6. Fixture-runner metadata where practical.

It is acceptable to leave runtime execution switches handwritten when they express real backend
behavior. It is not acceptable for command names, categories, packet body sizes, or JS constants to
drift across independent lists.

Do not create `spec/drp2/commands.yml` as a second hand-maintained command list unless the current
schema files cannot encode the required metadata cleanly. Preferred implementation order:

1. make the active command set in `spec/drp2/schema/README.md`, `drp_command.json`, and
   `commands/*.json` sufficient;
2. generate or validate C command enums, command names, packet phase metadata, fixed body sizes, and
   JS metadata from that authority;
3. keep handwritten backend execution switches only for behavior, not metadata;
4. remove or convert old switches/tables once generated metadata replaces them.

### WASM Scenario and Constants Authority

Use one registry source for browser-live scenarios and WASM-facing constants. The first candidate
authority is the existing example manifest:

```text
examples/c/MANIFEST.yaml
```

Generate or validate:

1. Scenario count.
2. Visual IDs.
3. Route names.
4. JS constants.
5. Smoke-test constants.

Remove hard-coded duplicated constants where practical. Preserve the intended WASM ABI unless the
break is explicitly covered by the API tiering phase.

Do not create `spec/wasm/scenarios.yml` as a second hand-maintained scenario list unless
`examples/c/MANIFEST.yaml` cannot represent the required ABI/export metadata. Preferred
implementation order:

1. extend `examples/c/MANIFEST.yaml` only if needed;
2. generate or validate `examples/webgpu/live_examples.js`, WASM scenario count/order, route IDs,
   smoke-test scenario constants, and exported JS constants against the manifest;
3. remove manual duplicate constants such as scenario counts and visual IDs where practical;
4. keep `src/wasm/scene_api.c` as implementation, not as the scenario registry authority.

### Panel Render Planner

Extract an internal panel render planner. Prefer a `DvzPanelRenderPlan` or equivalent internal
type. The planner owns:

1. Pass ordering.
2. Generated visual inclusion.
3. Visual attachment role filtering.
4. Transparency, depth peel, WBOIT, SSAO, and occlusion decisions.
5. Technique selection and render-node grouping decisions.

`src/scene/scene_emit/panel.c` should consume the plan instead of containing all of these decisions
inline. Keep the generated visual attachment policy centralized and extend architecture source
guards when useful.

This phase must preserve existing command-stream behavior unless a behavior change is explicitly
documented in the phase notes. Add focused tests or stream-shape assertions for pass ordering,
generated visual inclusion, attachment-role filtering, and technique grouping so the new planner is
not merely a second policy layer.

### Optional Scene Internals

Run this only after the required phases pass. Split `_scene.h` by subsystem and introduce typed
pools or object-store helpers only where doing so reduces duplicated capacity or lifetime logic.

Do not attempt a full dynamic-allocation rewrite unless it becomes obviously smaller and safer than
preserving the current fixed-array model.

This optional phase is not required for the autonomous run. Execute it only if Phases 1-5 are green
and the cleanup is mechanical enough to complete with one clear validation loop. Otherwise record it
as deferred.


## Required Phase Order

### Phase 0: Baseline and Authority Audit

Goal: establish the starting state and prevent accidental parallel systems.

Deliverables:

1. Baseline validation results for the commands in **Baseline Execution Rules**.
2. Short authority map for API status, render protocol types, DRP2 command metadata, WASM scenarios,
   and panel render planning.
3. Confirmation that any existing `data` submodule state is unstaged and will remain uncommitted.
4. No code changes unless baseline-only documentation is required.

Validation:

```sh
git status --short
git submodule status data
git diff --check
just build
just spec-check
```

Commit message, only if this phase changes tracked docs:

```text
pre-rc: record refactor baseline
```

### Phase 1: API Tiering

Goal: make public API status mechanical.

Deliverables:

1. `spec/api/status.yml`.
2. Stable/experimental/advanced/internal classifications for the active public headers.
3. Clarified default umbrella include policy.
4. Updated binding/docs generation policy where practical.
5. Drift check or generator integration proving docs/bindings do not maintain an independent API
   tier table.

Validation:

```sh
just build
just spec-check
python3 tools/check_example_manifests.py
git diff --check
```

Commit message:

```text
pre-rc: add mechanical api tiering
```

### Phase 2: Backend-Neutral Rendering Types

Goal: remove Vulkan-shaped public protocol leakage from scene and DRP2 APIs.

Deliverables:

1. Backend-neutral public rendering type header.
2. Internal Vulkan mapping for moved values.
3. Scene and DRP2 public docs/comments no longer describe public protocol values as `VkFormat`,
   `VkPrimitiveTopology`, or equivalent Vulkan contracts.
4. JS/WASM constants updated or generated from the same authority where practical.
5. Existing numeric values preserved where practical, but not documented as a public contract.
6. Old enum locations either removed as authorities or converted to forwarding includes without
   independent definitions.

Validation:

```sh
just build
just test drp2
just test scene
just spec-check
python3 tools/check_example_manifests.py
git diff --check
```

Commit message:

```text
pre-rc: make render protocol types backend neutral
```

### Phase 3: DRP2 Metadata Authority

Goal: replace duplicated command metadata switches and lists with one source of truth.

Deliverables:

1. DRP2 command metadata authority using the existing schema tree unless it is proven insufficient.
2. Generated or table-driven command names, kinds, fixed body sizes, packet metadata, and JS
   metadata where practical.
3. Updated fixture runner and packet tests.
4. Removal or validation of any old metadata switches/lists replaced by the new authority.

Validation:

```sh
just build
just test drp2
just spec-check
python3 tools/check_example_manifests.py
git diff --check
```

Commit message:

```text
pre-rc: centralize drp2 command metadata
```

### Phase 4: WASM Bridge Split

Goal: make the WASM scene bridge modular and registry-driven.

Deliverables:

1. Split `src/wasm/scene_api.c` into coherent implementation units.
2. Separate scenario registry, scene wrappers, input routing, artifact/packet export, query, and
   diagnostics logic.
3. Registry-backed or validated scenario/constants metadata.
4. Updated browser smoke and route metadata where needed.
5. Removal or generator validation of manual duplicate scenario counts, route IDs, visual IDs, and
   JS constants.

Validation:

```sh
just build
just wasm-scene-smoke
node --check tools/wasm_scene_smoke.mjs
python3 tools/check_example_manifests.py
git diff --check
```

Commit message:

```text
pre-rc: split wasm scene bridge
```

### Phase 5: Panel Render Planner

Goal: move panel render policy out of the monolithic emit path.

Deliverables:

1. Internal panel render plan type and planner implementation.
2. `scene_emit/panel.c` consumes the plan.
3. Generated visual attachment policy remains centralized.
4. Architecture source guards updated where useful.
5. Tests or stream-shape assertions proving the planner is the single render-policy authority for
   pass ordering, generated visuals, attachment-role filtering, and technique grouping.

Validation:

```sh
just build
just test scene
just spec-check
git diff --check
```

Commit message:

```text
pre-rc: extract panel render planning
```

### Optional Phase 6: Scene Internal State Cleanup

Goal: reduce `_scene.h` gravity without changing behavior.

Deliverables:

1. Subsystem-private internal headers for large scene state groups.
2. Typed pool/store helper only where it removes repeated capacity or lifetime logic.
3. No broad behavior rewrite.
4. No mixed old/new object-store paths; each migrated subsystem must have one lifetime/capacity
   authority.

Validation:

```sh
just build
just test scene
just spec-check
git diff --check
```

Commit message:

```text
pre-rc: split scene internal state
```


## Final Report Requirements

After all required phases complete, report:

1. Commit list and phase mapping.
2. Validation commands and outcomes.
3. Remaining API/status risks.
4. Any intentionally deferred optional cleanup.
5. Confirmation that no unapproved `data` submodule update, generated/runtime binary, vendored
   runtime library, large binary, or unrelated user change was committed.
6. Confirmation that replaced legacy authorities were removed, generated, or mechanically validated
   against the new single authority.


## Execution Log

### Phase 0: Baseline and Authority Audit

Status: complete.

Validation:

```sh
git status --short
git submodule status data
git diff --check
just build
just spec-check
```

Result:

1. `git status --short` reported only unstaged `data` submodule state: ` m data`.
2. `git submodule status data` reported `c1506b18196e509a9d50f26faba41d6838620aa8`.
3. `git diff --check` passed.
4. `just build` passed with no rebuild needed.
5. `just spec-check` passed, including scene shader ABI, DRP2 fixtures, WebGPU preflight, and
   source-guard checks.

Authority map:

1. API status: `spec/api/status.yml` to be created in Phase 1 and consumed by docs/bindings checks.
2. Render protocol types: `include/datoviz/render_types.h` to become the single public protocol
   enum owner in Phase 2.
3. DRP2 command metadata: existing `spec/drp2/schema/` tree first; do not add a parallel command
   manifest unless proven necessary.
4. WASM scenarios/routes/constants: existing `examples/c/MANIFEST.yaml` first; generate or validate
   browser/WASM registries from it.
5. Panel render planning: new internal planner becomes the single scene emit policy owner for pass
   ordering, generated visuals, attachment role filtering, and technique grouping.

Known risk: `data` remains dirty and unstaged; it must stay out of all checkpoint commits.

### Phase 1: API Tiering

Status: complete.

Validation:

```sh
just build
just spec-check
python3 tools/check_example_manifests.py
git diff --check
```

Result:

1. `just build` passed after rebuilding the public header C++ probe.
2. `just spec-check` passed and now includes `tools/check_api_status.py`.
3. `python3 tools/check_example_manifests.py` passed after regenerating
   `docs/examples/examples.json`.
4. `git diff --check` passed.

Changes:

1. Added `spec/api/status.yml` as the single API tiering manifest.
2. Added `tools/check_api_status.py` and wired it into `tasks/spec_check.py`.
3. Added `include/datoviz/advanced.h` for opt-in low-level/runtime APIs.
4. Reduced `include/datoviz/datoviz.h` to the stable/default umbrella.
5. Removed C API page-local status labels from `spec/api/C_API_REFERENCE_POLICY.yaml`; generated
   C API docs now derive page status from `spec/api/status.yml`.
6. Removed stale docs-policy globs for non-installed `ds` and `thread` headers.

Known risk: some stable headers still transitively expose advanced records until Phase 2 and later
boundary work reduce protocol leakage. This is tracked by the manifest and validator rather than by
prose comments.
