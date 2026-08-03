# Spec Index

This directory contains normative design material for v0.4 contracts and features, including implemented contracts, active proposals, informative roadmaps, and historical decisions whose authority is identified by their owning indexes.

Use `spec/` for contracts that should drive implementation and tests.
Do not use it for execution planning or broad architecture essays.


## Authority Map

- `architecture/`: cross-cutting source-module layers, scene split direction, and external
  dependency policy.
- `api/`: cross-module public API conventions for naming, structs, bindings, and release review.
- `bindings/`: raw generated Python `ctypes` binding architecture, generation policy, and
  validation rules.
- `build/`: component-target, packaging, install/export, and package-consumer build contracts.
- `data/`: data-submodule layout, manifest/provenance rules, and scientific showcase-dataset
  policy.
- `docs/`: v0.4 documentation architecture, example coverage, gallery site, and AI-friendly
  authoring rules.
- `drp2/`: backend-agnostic rendering protocol contract, schemas, fixtures, DVZR replay, and
  native-runtime pressure rules.
- `examples/`: executable example taxonomy, generated example-page contract, gallery selection,
  and scientific showcase policy.
- `release/`: release readiness, RC process, communication/blog assets, and gallery outreach
  policy.
- `scene/`: scene-layer requirements, consumer-side object model, retained object semantics, and
  implementation boundary notes.
- `testing/`: test-runner scheduling, skip, fixture, and validation policy.

AI-facing usage policy starts in `docs/AI_DOCUMENTATION.md`. Scene/app defaults, copy-safe examples,
diagnostic shape, and Python scope then route to `scene/api/`, `scene/examples/`,
`scene/validation/`, `api/`, and `bindings/` respectively. Cross-cutting module placement questions
route to `architecture/`; protocol replay questions route to `drp2/`.


## Rules

1. If violating a statement should fail implementation work or conformance tests, it belongs in `spec/`.
2. If a document is primarily about sequencing, backlog, or what to do next, it belongs in `agents/`.
3. If a document is primarily explanatory, comparative, or tutorial-like, it belongs in `docs/`.
4. Keep top-level `spec/` limited to this index and intentionally global policy; put owned material
   in the nearest topic directory.
5. Avoid speculative API sketches in `spec/` unless they are directly needed to validate the written
   contract.


## Validation

Spec-owned executable checks should be runnable from the repository root.

Current entrypoint:

```bash
just spec-check
```

This validates API status metadata, DRP2 command metadata and fixtures, the WASM bridge and WebGPU fixture path, scheduler behavior, scene query and architecture source guards, and the visual-family boundary. `just spec-check` also runs shader ABI and visual-boundary entry-point checks through the `just` dependency graph. Scene spec changes that alter implemented behavior should additionally run the narrowest relevant `just test scene` filter.
