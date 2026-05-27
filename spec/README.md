# Spec Index

This directory contains normative design material for v0.4 contracts and features, including some
surfaces that now have partial implementations.

Use `spec/` for contracts that should drive implementation and tests.
Do not use it for execution planning or broad architecture essays.


## Layout

- `drp2/`: backend-agnostic rendering protocol contract, schemas, fixtures, and native-runtime
  pressure rules
- `api/`: cross-module public API conventions for naming, structs, bindings, and release review
- `docs/`: v0.4 documentation architecture, example-coverage policy, and AI-friendly authoring
  rules
- `scene/`: scene-layer requirements, consumer-side object model, retained object semantics, and
  implementation boundary notes


## Rules

1. If violating a statement should fail implementation work or conformance tests, it belongs in `spec/`.
2. If a document is primarily about sequencing, backlog, or what to do next, it belongs in `agents/`.
3. If a document is primarily explanatory, comparative, or tutorial-like, it belongs in `docs/`.
4. Avoid speculative API sketches in `spec/` unless they are directly needed to validate the written
   contract.


## Validation

Spec-owned executable checks should be runnable from the repository root.

Current entrypoint:

```bash
just spec-check
```

At the moment this validates the DRP2 fixture corpus and the DRP2 fixture-runner tests.
Scene spec changes that alter implemented behavior should also run the narrowest relevant
`just test scene` filter, because the scene spec now has multiple active source slices.
