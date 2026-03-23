# DRP2 Fixtures

This directory contains canonical DRP2 conformance fixtures for the active `2.0` contract.

Fixtures are both:

1. authoritative machine-readable test vectors,
2. worked examples that explain how the spec should be interpreted.


## Layout

The first corpus is split into:

1. `negative/`
2. `positive/`
3. `negative_schema/`

The immediate priority is `negative/`.
Those fixtures lock the failure boundary for the active DRP2 `2.0` command set, schemas, error
codes, and lifetime/state rules.


## First Corpus

The first negative corpus should stay intentionally small and cover the core validation surface:

1. duplicate id rejection,
2. unknown id rejection,
3. wrong object type rejection,
4. draw outside render pass,
5. dispatch outside compute pass,
6. copy inside a pass,
7. finishing an encoder with an open pass,
8. pass-kind mismatch,
9. destroying a resource still referenced by recorded work,
10. buffer range violation,
11. texture range violation,
12. missing command discriminator,
13. wrong field names,
14. missing required command fields.

Positive fixtures can follow once the negative corpus and fixture envelope are frozen.

The first positive corpus should stay minimal and focus on clean command shapes:

1. buffer upload only,
2. texture upload only,
3. command encoder plus compute pass plus successful finish.


## Metadata Policy

Fixtures may include human-readable metadata such as:

1. `description`
2. `reason`
3. `fix`
4. `notes`

These fields are non-normative.
They exist to make the corpus readable and reviewable.
Only the normative fields described in `FORMAT.md` determine conformance.


## Schema-Negative Policy

Fixtures under `negative_schema/` are expected to fail during `schema_validation`.

Those fixtures must still satisfy the fixture envelope itself.
Only the embedded DRP command objects are intentionally malformed.


## Reuse

Fixtures should remain backend-agnostic and reusable by both native and browser runtimes.

See `FORMAT.md` for the fixture envelope and naming rules.
See `schema/drp_fixture.schema.json` for the machine-readable fixture envelope.
