# Pre-RC Architecture Refactor Plan

Status: approved maintainer handoff for autonomous execution.
Updated: 2026-07-01.

This plan records the manual architecture decisions required before an agent performs broad
pre-RC refactoring. A future high-reasoning Codex agent may execute this document end to end with
checkpoint commits, without asking for routine implementation choices.


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
7. Keep generated/runtime binary payloads out of commits unless the user explicitly approves those
   exact files in the current turn.
8. Never stage or commit the `data` submodule pointer unless the user explicitly approves that
   submodule commit or pointer update in the current turn.
9. Treat staged `data` gitlink updates as stop signs.
10. Ignore unstaged/untracked `data` working-tree state only if it remains unstaged and uncommitted.


## Stop Conditions

The executing agent should proceed autonomously except in these cases:

1. A public API name has two plausible long-term choices not decided in this document.
2. Tests fail for reasons that appear unrelated to the current phase after investigation.
3. The staged set contains `data`, generated/runtime binaries, vendored runtime libraries, large
   binaries, or unrelated user changes.
4. The implementation must widen beyond the phase scope to remain correct.
5. A required code-generation path would make normal builds depend on unavailable tools.

If a stop condition occurs, record the exact blocker, current diff state, and preferred next move.


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

`include/datoviz/datoviz.h` should become the stable/default umbrella. Low-level DRP2, vklite, vk,
window, stream, and runtime APIs should be opt-in through explicit headers, with an
`include/datoviz/advanced.h` convenience umbrella if it simplifies migration.

Do not rely on prose-only "advanced/unstable" comments as the only API boundary. The manifest
should guide headers, generated docs, raw-binding policy, top-level Python facade policy, and symbol
export review where practical.

### Backend-Neutral Rendering Types

Move public rendering protocol types out of Vulkan-facing headers. Prefer:

```text
include/datoviz/render_types.h
```

This header should own public types such as `DvzFormat`, primitive topology, compare op, cull mode,
front face, blend factors, blend ops, or closely related protocol enums needed by scene and DRP2.
Vulkan and WebGPU mappings must remain internal to their backend/runtime layers.

Preserve existing numeric values for moved enums where practical and low-risk, especially for
`DvzFormat`, to reduce transition churn and avoid unnecessary fixture/binding diffs. This numeric
preservation is incidental compatibility, not a public contract. Do not document Vulkan numeric
identity as part of the Datoviz API, and do not design new APIs that depend on it.

### Compatibility Stance

Source/API breaks are allowed now when they remove accidental backend leakage, accidental exports,
or bad ownership boundaries. Do not add ABI compatibility aliases. Add temporary source aliases only
when they are trivial, clearly deprecated, and do not pollute the new architecture.

### DRP2 Command Authority

Create one DRP2 command schema, preferably:

```text
spec/drp2/commands.yml
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

### WASM Scenario and Constants Authority

Use one registry source for browser-live scenarios and WASM-facing constants. If an existing
examples manifest is already authoritative, extend it. Otherwise create:

```text
spec/wasm/scenarios.yml
```

Generate or validate:

1. Scenario count.
2. Visual IDs.
3. Route names.
4. JS constants.
5. Smoke-test constants.

Remove hard-coded duplicated constants where practical. Preserve the intended WASM ABI unless the
break is explicitly covered by the API tiering phase.

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

### Optional Scene Internals

Run this only after the required phases pass. Split `_scene.h` by subsystem and introduce typed
pools or object-store helpers only where doing so reduces duplicated capacity or lifetime logic.

Do not attempt a full dynamic-allocation rewrite unless it becomes obviously smaller and safer than
preserving the current fixed-array model.


## Required Phase Order

### Phase 1: API Tiering

Goal: make public API status mechanical.

Deliverables:

1. `spec/api/status.yml`.
2. Stable/experimental/advanced/internal classifications for the active public headers.
3. Clarified default umbrella include policy.
4. Updated binding/docs generation policy where practical.

Validation:

```sh
just build
just spec-check
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

Validation:

```sh
just build
just test drp2
just test scene
just spec-check
git diff --check
```

Commit message:

```text
pre-rc: make render protocol types backend neutral
```

### Phase 3: DRP2 Metadata Authority

Goal: replace duplicated command metadata switches and lists with one source of truth.

Deliverables:

1. DRP2 command schema or central table.
2. Generated or table-driven command names, kinds, fixed body sizes, packet metadata, and JS
   metadata where practical.
3. Updated fixture runner and packet tests.

Validation:

```sh
just build
just test drp2
just spec-check
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

Validation:

```sh
just build
just wasm-scene-smoke
node --check tools/wasm_scene_smoke.mjs
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
