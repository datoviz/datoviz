# Spec Index

This directory contains normative design material for features that are not yet fully implemented.

Use `spec/` for contracts that should drive implementation and tests.
Do not use it for execution planning or broad architecture essays.


## Layout

- `drp2/`: backend-agnostic rendering protocol contract, schemas, and fixtures
- `scene/`: future scene-layer requirements and consumer-side object model


## Rules

1. If violating a statement should fail implementation work or conformance tests, it belongs in `spec/`.
2. If a document is primarily about sequencing, backlog, or what to do next, it belongs in `agents/`.
3. If a document is primarily explanatory, comparative, or tutorial-like, it belongs in `docs/`.
4. Avoid speculative API sketches in `spec/` unless they are directly needed to validate the written
   contract.
