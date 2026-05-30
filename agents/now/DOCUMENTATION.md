# Datoviz v0.4 Documentation Plan

Status: active documentation roadmap. Updated: 2026-05-30.

Keep API, scene, DRP2, and documentation architecture contracts in `spec/`. Keep this file focused
on release documentation gates.


## Source-Of-Truth Boundaries

1. Release sequencing: [RELEASE.md](RELEASE.md).
2. Current blockers: [STATUS.md](STATUS.md).
3. Public API conventions: [../../spec/api/PUBLIC_API_CONVENTIONS.md](../../spec/api/PUBLIC_API_CONVENTIONS.md).
4. Python/GSP scope: [../../spec/api/PYTHON_GSP_SCOPE.md](../../spec/api/PYTHON_GSP_SCOPE.md).
5. Scene API and semantics: [../../spec/scene/](../../spec/scene/).
6. DRP2 commands, schemas, fixtures, and runtime contracts: [../../spec/drp2/](../../spec/drp2/).
7. Documentation IA, example coverage, and AI-friendly docs rules: [../../spec/docs/](../../spec/docs/).
8. Release readiness, RC process, communication, and gallery outreach:
   [../../spec/release/](../../spec/release/).

The public `docs/` tree may be rebuilt in place for v0.4. Do not recreate the v0.3 Python-first
guide as current Datoviz documentation.


## Required Public Docs

1. README that presents v0.4 as the current branch surface.
2. Install/build guide with supported platforms, dependencies, CMake, and `just` commands.
3. Feature/status table using `supported`, `experimental`, `advanced/unstable`, `deferred`, and
   `external/GSP`.
4. Known issues and unsupported variants.
5. Datoviz/GSP/VisPy2 positioning note.
6. RC and final release notes.
7. Example/gallery index with screenshots or captured artifacts where appropriate.
8. Generated C reference or complete API reference outline.
9. Raw `ctypes` binding documentation.
10. WebGPU/WASM experimental-scope documentation.
11. Compute+graphics experimental-scope documentation, including the portable DRP2 subset, native
    synchronization boundary, and optional CUDA SDK example status.
12. Gallery/data attribution policy for public datasets, generated media, and reuse in release
    communication.
13. Short contributor guidance for AI-assisted docs/example work: page type, source of truth,
    validation command, and unsupported-feature status.


## RC1 Gate

Required:

1. public header inventory;
2. public surface/status table;
3. ownership and destroy-rule notes for public objects;
4. callback, polling, and readback lifetime notes;
5. raw `ctypes` scope;
6. WebGPU/WASM experimental scope and known gaps;
7. compute+graphics experimental scope and CUDA/CuPy boundary;
8. clear statement that old high-level Pythonic Datoviz API migration is outside v0.4 Datoviz
   scope.

The raw `ctypes` scope page may land before the generated API reference is complete. For RC1, it
only needs to define import style, generated-binding status, ownership and callback lifetime rules,
validation commands, and the boundary with GSP/VisPy2. Do not hand-maintain exhaustive C or raw
binding symbol catalogs in prose.


## RC2 Gate

Required:

1. documentation structure mostly final;
2. generated C reference or complete outline;
3. usable raw `ctypes` docs;
4. release examples with captured artifacts;
5. render-conformance or fixture result linked;
6. known issues updated from RC1 feedback.
7. real-dataset showcase candidates reviewed for license, attribution, scientific context, and
   whether the visualization would be useful to the dataset authors.
8. gallery media generation path documented for screenshots, animated GIFs, or short videos.

The generated C/raw API outline should come from parsed public headers. The first candidate source
is `build/bindings/datoviz_api.json`, produced by the raw-binding API extraction pipeline.


## Final Gate

Required:

1. final feature table;
2. final known issues and limitations;
3. install/build and positioning notes published;
4. raw `ctypes` and WebGPU/WASM scopes published;
5. website, gallery, and release announcement assets published.
6. public dataset examples include source links, license/citation notes, prepared-data provenance,
   and any required permissions.
7. direct outreach drafts for selected dataset authors are reviewed before sending.


## Validation

Documentation-only:

```sh
git diff --check
git status --short
```

Generated references, examples, screenshots, or inventories need the narrowest relevant generation,
build, or smoke command recorded in the final note.
