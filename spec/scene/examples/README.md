# Scene Examples

This directory contains informative worked examples and gallery planning notes for the active v0.4
scene stack. The examples are pressure tests, not normative API sources. Promote release
commitments through [PLANNING.md](PLANNING.md), and keep canonical behavior in the main scene,
DRP2, API, and validation specs.

The scenario files were aggressively compressed in commit `81126893b` from the previous
domain-folder layout. If detailed historical per-example notes are needed, inspect the parent of
that commit, for example:

```bash
git show 81126893b^:spec/scene/examples/geo/SHOWCASE_WIND_FIELD.md
git ls-tree -r --name-only 81126893b^ spec/scene/examples
```

Treat those historical files as provenance, not active planning. Promote any still-useful detail
back into the current scenario bundles or [PLANNING.md](PLANNING.md) before relying on it.


## Main Files

| File | Role |
| --- | --- |
| [CATALOG.md](CATALOG.md) | Scenario ID lookup table with stage and owning bundle. |
| [PLANNING.md](PLANNING.md) | Release staging, gallery priorities, current support gaps, and pickup order. |
| [FIXTURES.md](FIXTURES.md) | Compact one-feature fixtures and generated DRP2/WebGPU/runtime validation ideas. |
| [ORGANIZATION.md](ORGANIZATION.md) | Cross-repository ownership, example lanes, scenario IDs, and metadata conventions. |
| [POLICIES.md](POLICIES.md) | Shared API caveats, data/cache rules, FramePlan/DRP2 references, and metadata block rules. |
| [PORTABLE_SCENARIO_RUNNER.md](PORTABLE_SCENARIO_RUNNER.md) | Write-once C scenario architecture for native Vulkan and WASM/WebGPU example hosts. |
| [STYLE.md](STYLE.md) | Gallery visual identity, screenshots, videos, typography, and accessibility guidance. |
| [EXECUTION.md](EXECUTION.md) | One-at-a-time migration loop, old-example handling, metadata, and validation workflow. |
| [TECHNIQUES.md](TECHNIQUES.md) | Cross-cutting rendering-technique notes that apply to multiple scenarios. |
| [DECISIONS.md](DECISIONS.md) | Historical decisions from the C example duplication/API cleanup. |
| [TEMPLATE.md](TEMPLATE.md) | Starting point for new scenario specs. |


## Scenario Layout

Scenario files live under [scenarios/](scenarios/) by release/action status:

| Directory | Meaning |
| --- | --- |
| `v04_required/` | Examples that are release narrative items, release proof, or required C smoke paths. |
| `v04_experimental/` | Stretch or backend-limited examples that can appear with explicit experimental status. |
| `v05/` | Important next-release examples that should not distort v0.4. |
| `later/` | Strategic pressure tests beyond v0.5. |
| `external_gsp/` | Workflows primarily owned by GSP/VisPy2/Matplotlib, with Datoviz keeping low-level fixtures. |
| `api_sketches/` | API-pressure sketches that are useful for design, but not release promises by themselves. |

Domain labels such as neuroscience, geo, molecular, dashboards, and compute belong in scenario
metadata instead of the directory structure. This keeps the folder organized by what future agents
actually need to decide: implement now, keep as fixture, defer, or hand off.


## Agent-Copyable Examples

An example is agent-copyable only when it is a complete, current starting point for user code. Mark
that status in scenario metadata or the generated example manifest instead of relying on prose.

Agent-copyable examples should:

1. use the public `scene`/`app` path unless the example explicitly targets DRP2 or runtime internals;
2. declare the demonstrated visual family or feature;
3. keep setup, data binding, rendering, and cleanup visible;
4. include or link the narrow validation command;
5. state backend, platform, and optional dependency limits;
6. avoid old v0.3 API names and invented plotting helpers;
7. link to the relevant visual, feature, or diagnostics spec.

Use these roles in metadata where possible:

| Role | Meaning |
| --- | --- |
| `minimal` | Smallest complete source for one visual or feature; safe agent default. |
| `update` | Demonstrates retained data updates, partial uploads, or streaming. |
| `offscreen` | Demonstrates offscreen rendering, capture, or readback. |
| `interaction` | Demonstrates controllers, picking, probing, selection, or callbacks. |
| `technique` | Demonstrates a rendering technique with limited composition. |
| `showcase` | Attractive composed example; not a minimal starting point. |
| `sketch` | API pressure test or future design sketch; not copy-safe. |


## Editing Rules

1. Keep scenario files short and specific to the scenario.
2. Put repeated cache/download/API caveats in [POLICIES.md](POLICIES.md).
3. Put release status, priority, and blockers in [PLANNING.md](PLANNING.md).
4. Put visual/screenshot/video direction in [STYLE.md](STYLE.md).
5. Put old-example migration and per-example execution workflow in [EXECUTION.md](EXECUTION.md).
6. Use scenario IDs from [ORGANIZATION.md](ORGANIZATION.md) when a runnable example, fixture, or
   generated gallery asset is created.
7. Preserve agent-copyable status when editing examples; downgrade it if the example becomes a
   sketch, showcase, or backend-limited pressure test.
