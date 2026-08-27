# GUI ImPlot And Docking Handoff

Status: planned non-blocking pre-RC3 candidate; implementation begins only after PR #145 merges. Updated: 2026-08-27.

This handoff routes the implementation of [GUI_EXTENSIONS_AND_DOCKING.md](../../spec/architecture/GUI_EXTENSIONS_AND_DOCKING.md). The durable specification is authoritative; this file owns sequencing, release boundaries, and checkpoint evidence only.

## Objective

Promote the experimental mixed ImPlot example into official default-on GUI support using one pinned offline ImGui/cimgui/ImPlot/cimplot family, Datoviz-owned paired contexts, an installed raw native C surface, and an opaque declarative docking layout tree. Preserve native C++ implementation with curated and generated C boundaries; do not rewrite the GUI implementation through cimgui.

## Release Boundary

This work is an optional pre-RC3 implementation lane, not an RC3 release blocker. It may use time while required provider work is externally blocked, but it must not delay exact-candidate freeze, packaging proof, hosted validation, or release publication.

The lane either lands completely before candidate freeze with all affected build, install, package, binding, GUI, and platform gates rerun, or remains deferred. At the cutoff, incomplete work must not be partially merged: retain the experimental default-off source example from PR #145 and move the official integration after RC3.

Landing the default-on dependency changes the exact candidate and invalidates earlier build/package evidence. Freeze and artifact proof begin only after the complete integration is merged or explicitly deferred.

## Entry Conditions

1. PR #145 is merged with the requested idle-render regression, default-off network-fetched example, and honest gallery media state.
2. The durable architecture specification and its dependency-policy cross-reference are integrated on the active branch.
3. The exact compatible ImGui/cimgui/ImPlot/cimplot source family and license/provenance plan are reviewed before dependency pointers or vendored sources change.
4. The worktree and staged set exclude unapproved `data`, paper artifacts, runtime binaries, and unrelated user changes.

## Checkpoint 0: Dependency Family And Build Contract

1. Admit one pinned offline ImPlot/cimplot source family compatible with the existing `external/cimgui` and nested ImGui implementation; never compile a second cimgui or ImGui copy.
2. Replace the example-only option with dependent `DVZ_BUILD_IMPLOT`, defaulting effectively on with GUI and off without GUI.
3. Export effective `DVZ_HAS_IMPLOT`, implement a clean explicit-off profile, and remove configure-time network access.
4. Record exact revisions, generated-wrapper provenance, licenses, notices, source-archive contents, and supported override policy.
5. Prove default-on, explicit-off, offline, shared, static, CMake-package, and pkg-config consumers before committing the dependency checkpoint.

Commit boundary: pinned family, build targets/options, install/export metadata, notices, and build-profile tests agree without adding runtime context or docking behavior.

## Checkpoint 1: ImPlot Context And Raw C Surface

1. Make each `DvzGui` own one paired `ImPlotContext` when available, created after a current ImGui context and destroyed before it.
2. Introduce scoped paired-context activation and restoration around Datoviz GUI boundaries and callbacks.
3. Compile ImPlot and cimplot against the exact Datoviz ImGui/cimgui types and symbols; do not add another renderer or frame lifecycle.
4. Add guarded `datoviz/implot.h`, install `cimplot.h` only when enabled, and include the wrapper only through the capability-guarded advanced surface.
5. Keep raw cimplot out of generated ctypes, NumPy adaptation, WASM, and stable `dvz_*` ABI policy.
6. Add C compile/link, installed-consumer, lifecycle, failure-unwind, multiple-context, recreation, and high-density draw-list tests.

Commit boundary: default-on context ownership and native C access pass focused and installed-package validation with no docking or example-private context remaining in this checkpoint.

## Checkpoint 2: Declarative Docking Layout

1. Split docking into a focused private implementation that exclusively owns `imgui_internal.h`, DockBuilder calls, generated IDs, and persistence lowering.
2. Add opaque layout and node identities, draft validation, explicit split results, typed initial sizes, stable window keys, and an identity-aware window-begin contract.
3. Distinguish pass-through native underlays from occupied GUI viewport leaves; docking must not implicitly reserve or resize scene panels.
4. Store validated lowering plans on `DvzGui` and apply them only after dockspace submission and before application windows begin.
5. Preserve compatible persisted user layouts, require schema-version bumps for authored topology changes, and support explicit one-shot reset without per-frame reconstruction.
6. Test invalid drafts, stale handles, deterministic topology, multiple GUI identities, persistence, migration, reset, disabled docking, and idle behavior.

Commit boundary: public code no longer reconstructs private dockspace IDs, and topology/persistence tests pass independently of the ImPlot example.

## Checkpoint 3: Example Migration And Default-On Completion

1. Migrate the mixed ImPlot example to Datoviz-owned context lifecycle, guarded raw cimplot calls, stable window identity, and the public dock-layout tree.
2. Remove example-owned ImPlot context management, `imgui_internal.h`, the private dockspace string, and example-specific source configuration.
3. Remove the old side-slot helper before API freeze when no supported consumer remains.
4. Regenerate example manifests, documentation, and gallery state; promote media only through the approved canonical pipeline.
5. Run native interaction, deterministic capture, idle scheduling, bindings, docs, source archive, installed consumer, Linux, macOS, Windows, and exact candidate gates.

Commit boundary: the official default-on integration, migrated example, generated surfaces, and affected release evidence are coherent.

## Cutoff Decision

Before RC3 candidate freeze, record one binary decision in [STATUS.md](STATUS.md): integrated and fully validated, or deferred intact until after RC3. Do not carry a partially merged dependency, context, docking, or packaging slice into the exact candidate.

## Validation Defaults

Each checkpoint runs its narrowest tests while iterating, followed by the gates named in the durable specification. Before every commit run `git diff --check`, `git status --short`, and `git diff --cached --stat`; never stage unapproved `data`, paper artifacts, generated runtime libraries, binaries, or unrelated user changes.
