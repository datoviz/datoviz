# GUI ImPlot And Docking Handoff

Status: deferred intact until after RC3; PR #145 merged and the implementation entry gate remains open for later work. Updated: 2026-08-30.

This handoff routes the implementation of [GUI_EXTENSIONS_AND_DOCKING.md](../../spec/architecture/GUI_EXTENSIONS_AND_DOCKING.md). The durable specification is authoritative; this file owns sequencing, release boundaries, and checkpoint evidence only.

## Objective

Promote the experimental mixed ImPlot example into official default-on GUI support using one pinned offline ImGui/cimgui/ImPlot/cimplot family, Datoviz-owned paired contexts, an installed raw native C surface, and an opaque declarative docking layout tree. Preserve native C++ implementation with curated and generated C boundaries; do not rewrite the GUI implementation through cimgui.

## Release Boundary

This work is deferred until after RC3. The current experimental example remains default-off, and RC3 admits no dependency upgrade, paired-context lifecycle, installed raw surface, declarative docking API, or partial migration from this lane.

The atomic official ImPlot slice and the declarative docking slice remain independently admissible after RC3, including as two pull requests. Each slice must land completely with its affected build, install, package, binding, GUI, and platform gates rerun. A completed ImPlot slice may land while docking remains deferred, in which case the transitional private example docking and compatibility helper remain explicitly temporary.

Landing the default-on dependency changes package contents and invalidates earlier build evidence. Its later release campaign begins only after the complete integration is merged.

## Entry Conditions

1. Complete: PR #145 merged at `7f1c6c65f` with the requested idle-render regression, default-off network-fetched example, and honest gallery media state.
2. The durable architecture specification and its dependency-policy cross-reference are integrated on the active branch.
3. The exact compatible ImGui/cimgui/ImPlot/cimplot source family and license/provenance plan are reviewed before dependency pointers or vendored sources change.
4. The worktree and staged set exclude unapproved `data`, paper artifacts, runtime binaries, and unrelated user changes.

## Completed Preimplementation Audit

The 2026-08-27 read-only audit selected this upstream base family:

| Layer | Upstream base | Identity |
| --- | --- | --- |
| cimgui | `0e533fd0b70f6add19825bea83b66743d5b8d95b` | `1.92.7dock` |
| Dear ImGui | `f5f6ca07be7ce0ea9eed6c04d55833bac3f6b50b` | `1.92.7-docking` |
| cimplot | `75a03832860f7832712cb5ad8d6e3ad6b69dd97c` | `v1.0` |
| ImPlot | `524f9fcd48d76c13fdf94c5ffbba8787a1ff7e39` | `v1.0` |

The current Datoviz cimgui 1.91.9b generated type schema is incompatible with stable cimplot v1.0. The selected stable family passed an out-of-tree raw C `BeginPlot`/`PlotLine`/`EndPlot` frame on local Apple Silicon and as an x86_64 executable under Rosetta; this is feasibility evidence, not a substitute for hosted or physical platform validation.

The Datoviz forks must carry three recorded changes: adapt and regenerate the range-slider extension for Dear ImGui 1.92.7; apply cimplot's `FormatSpec[16]` bounds correction following upstream fix `125034d782adaa43e840c0f1997aba64fbe2043f`; and correct generated Windows export/import/static declaration policy. Both projects remain MIT-licensed, with their nested Dear ImGui and ImPlot licenses included in source and package evidence.

Native Datoviz currently builds a shared library only. General native static packaging is outside this lane; do not claim or create a static-package gate here.

## Checkpoint 0: Atomic Official ImPlot Slice

1. Fork and pin the audited cimgui/ImGui/cimplot/ImPlot family, including regenerated wrapper provenance and the three bounded patches; never compile a second cimgui or ImGui copy.
2. Use explicit source targets for Dear ImGui, cimgui, ImPlot, and cimplot, remove configure-time downloads, update CI submodule lists and source-archive ownership, and prove one implementation of each symbol family.
3. Add dependent `DVZ_BUILD_IMPLOT`, a clean explicit-off profile, and a generated installed build-configuration header whose effective feature state agrees with CMake, pkg-config, and wheel metadata.
4. Make each `DvzGui` own one paired ImPlot context, add safe paired activation/restoration and destruction, and forbid example-owned manipulation of Datoviz contexts.
5. Add guarded `datoviz/implot.h`, install the exact raw headers when enabled, keep cimplot out of generated Python/WASM bindings, and implement correct Windows export/import semantics.
6. Migrate the experimental example away from fetched ImPlot sources and example-owned context while allowing its transitional private docking code to remain until Checkpoint 2.
7. Prove raw C runtime plotting, context failure unwind and multiple-instance lifecycle, default-on and explicit-off offline builds, native shared CMake and pkg-config consumers, wheel/source packaging, duplicate-symbol absence, Windows exports, and ARM64 coverage where available.

Integration boundary: development may use internal checkpoint commits, but the active release line receives dependency ownership, lifecycle, raw installed surface, minimal example migration, `DVZ_HAS_IMPLOT=1`, removal of the example-only option, and the default-on flip as one coherent unit. Do not merge a dependency-only or advertised symbol-only intermediate state.

## Checkpoint 1: Declarative Docking Layout

1. Split docking into a focused private implementation that exclusively owns `imgui_internal.h`, DockBuilder calls, generated IDs, and persistence lowering.
2. Add opaque layout and node identities, draft validation, explicit split results, typed initial sizes, stable window keys, and an identity-aware window-begin contract.
3. Distinguish pass-through native underlays from occupied GUI viewport leaves; docking must not implicitly reserve or resize scene panels.
4. Store validated lowering plans on `DvzGui` and apply them only after dockspace submission and before application windows begin.
5. Preserve compatible persisted user layouts, require schema-version bumps for authored topology changes, and support explicit one-shot reset without per-frame reconstruction.
6. Test invalid drafts, stale handles, deterministic topology, multiple GUI identities, persistence, migration, reset, disabled docking, and idle behavior.

Commit boundary: public code no longer reconstructs private dockspace IDs, and topology/persistence tests pass independently of the ImPlot example.

## Checkpoint 2: Docking Migration And Completion

1. Migrate the mixed ImPlot example from transitional private docking code to stable window identity and the public dock-layout tree.
2. Remove `imgui_internal.h` and the private dockspace string from the example; context management and example-specific source configuration were already removed in Checkpoint 0.
3. Keep the old side-slot helper as a wrapper over the declarative model while `example_tuner.c` or another supported consumer uses it; removal requires a separate migration and API decision.
4. Regenerate example manifests, documentation, and gallery state; promote media only through the approved canonical pipeline.
5. Run native interaction, deterministic capture, idle scheduling, bindings, docs, source archive, installed consumer, Linux, macOS, Windows, and exact candidate gates.

Commit boundary: the official default-on integration, migrated example, generated surfaces, and affected release evidence are coherent.

## RC3 decision

Both slices are deferred intact until after RC3. Do not carry a partially merged dependency, context, raw surface, docking model, or packaging slice into the RC3 candidate.

## Validation Defaults

Each checkpoint runs its narrowest tests while iterating, followed by the gates named in the durable specification. Before every commit run `git diff --check`, `git status --short`, and `git diff --cached --stat`; never stage unapproved `data`, paper artifacts, generated runtime libraries, binaries, or unrelated user changes.
