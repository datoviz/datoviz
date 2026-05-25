# Datoviz v0.4 Documentation Plan

> **Execution Status**
> - **Status:** `ACTIVE DOCUMENTATION ROADMAP`
> - **Updated on:** `2026-05-25`
> - **Purpose:** collect the v0.4 public-documentation deliverables, release gates, and
>   source-of-truth links in one place
> - **Audience:** maintainers and agents preparing the v0.4 API/docs inventory, release
>   candidates, gallery, and final public documentation

This file is the active execution plan for v0.4 documentation. It collects the documentation work
that used to be scattered across release and status notes. Keep detailed API, scene, and DRP2
contracts in `spec/`; keep this file focused on what documentation must exist for RC1, RC2, and
the final release.


## Source-Of-Truth Boundaries

1. Active release sequencing belongs in [RELEASE.md](RELEASE.md).
2. Current implementation blockers belong in [STATUS.md](STATUS.md).
3. Branch orientation belongs in [START.md](START.md).
4. Normative public API rules belong in
   [../../spec/api/PUBLIC_API_CONVENTIONS.md](../../spec/api/PUBLIC_API_CONVENTIONS.md).
5. Normative scene API shape belongs in
   [../../spec/scene/api/API_SURFACE.md](../../spec/scene/api/API_SURFACE.md) and installed
   headers under `include/datoviz/`.
6. Scene semantics, visual contracts, frame planning, interaction, annotations, scales, and runtime
   boundaries belong under [../../spec/scene/](../../spec/scene/).
7. DRP2 commands, schemas, fixtures, conformance, and runtime contract details belong under
   [../../spec/drp2/](../../spec/drp2/).
8. User-facing tutorial, guide, and reference material should move to `docs/` only once the v0.4
   public documentation migration has explicitly started.

Do not copy detailed API rules or visual semantics into this file. Link to the owning spec instead.


## Public Documentation Deliverables

The v0.4 public documentation set should include:

1. README update that presents v0.4 as the current branch surface, not legacy v0.3.
2. install and build guide with supported platforms, required dependencies, and CMake/just commands.
3. feature/status table that classifies each visible capability as `supported`, `experimental`,
   `advanced/unstable`, `deferred`, or `external/GSP`.
4. known issues and limitations, including explicit unsupported visual and runtime variants.
5. v0.3-to-v0.4 migration/status notes focused on visible capability parity, not source
   compatibility.
6. release notes for RCs and final `v0.4.0`.
7. minimal RC1 user guide outside `docs/` until the full v0.4 public documentation migration starts.
8. example and gallery index with screenshots or captured artifacts for release examples.
9. generated C reference or complete API reference outline.
10. raw `ctypes` binding documentation, including generation scope, loading expectations, and the
   boundary with higher-level Python.
11. WebGPU/WASM experimental-scope documentation with supported subset, diagnostics, and known gaps.
12. GSP/VisPy2 positioning note that keeps high-level Python plotting outside the Datoviz v0.4
    release surface.


## API Inventory Deliverables

The API/docs inventory is an RC1 blocker. It should produce:

1. a public header inventory for `include/datoviz/`;
2. a symbol/module classification table using the release labels;
3. ownership and destroy-rule notes for public objects;
4. callback lifetime and polling/readback lifetime notes;
5. error/status behavior notes for public or recoverable failure paths;
6. feature-gated API notes for optional backends and generated bindings;
7. an internal-leakage list for public headers that still expose implementation details;
8. deferred or advanced/unstable labels for lower-layer surfaces such as `drp2`, `vk`, `vklite`,
   `canvas`, `stream`, and `video` when they are not the main public narrative.

The inventory should reference the installed headers for exact names and signatures. If a spec and a
header disagree, fix the spec or header explicitly instead of documenting both as current.


## Release Gates

### RC1: API And Status Candidate

Required documentation state:

1. public API inventory exists;
2. supported, experimental, advanced/unstable, deferred, and external/GSP labels are visible;
3. v0.3 visible parity table exists and every gap has a fix/defer/GSP disposition;
4. raw `ctypes` scope is documented enough for early testers;
5. WebGPU/WASM is documented as experimental, with known gaps;
6. a minimal RC1 user guide is linked from `README.md` and the GitHub pre-release body;
7. public headers and docs no longer imply obsolete v0.3 APIs are current.


### RC2: Documentation And Gallery Candidate

Required documentation state:

1. website or documentation structure is mostly final;
2. generated C reference or complete API reference outline exists;
3. raw `ctypes` documentation is usable;
4. release examples have screenshots or captured artifacts;
5. minimal render-conformance fixture result is linked;
6. known issues are updated from RC1 feedback.

Exit criterion: a new user can understand what v0.4 is, install or build it, run examples, and see
the intended C, Python-binding, and WebGPU scope.


### Final v0.4.0

Required documentation state:

1. feature table is final for the release;
2. known issues and limitations are explicit;
3. migration notes and install instructions are published;
4. raw `ctypes` and WebGPU/WASM experimental scopes are published;
5. GSP/VisPy2 positioning is published;
6. website, gallery, and release announcement assets are published.


## Example And Gallery Documentation

Use
[../../spec/scene/examples/EXAMPLE_ORGANIZATION.md](../../spec/scene/examples/EXAMPLE_ORGANIZATION.md)
for example ownership and layout, and
[../../spec/scene/examples/EXAMPLE_RELEASE_STAGING.md](../../spec/scene/examples/EXAMPLE_RELEASE_STAGING.md)
for release staging.

The release gallery should prove the declared v0.4 feature set with a compact set of native C
examples first. Raw Python examples should stay close to the generated binding surface. High-level
Python plotting examples belong to GSP/VisPy2 unless they are deliberately documenting raw Datoviz
bindings.


## Validation Defaults

For documentation-only edits:

1. `git diff --check`
2. inspect `git status --short`

For generated references, examples, screenshots, or API inventories, add the narrowest relevant build,
generation, or smoke command to the final documentation record.
