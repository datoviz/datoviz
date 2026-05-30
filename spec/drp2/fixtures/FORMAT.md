# DRP2 Fixture Format

This document defines the authoritative shape of the first DRP2 `2.0` conformance fixtures.

Fixtures serve two roles:

1. machine-checkable conformance vectors,
2. human-readable worked examples of the DRP2 contract.

The machine-checkable fields are authoritative.
Human-oriented metadata fields are non-normative and must not affect pass/fail outcomes.


## Goals

1. backend-agnostic input representation,
2. versioned fixture metadata,
3. support for both positive and negative cases,
4. support for deterministic replay expectations,
5. readable examples that explain why a case should pass or fail.


## Representation

The first fixtures use JSON for readability.

If a binary encoding is introduced later, the JSON fixtures should remain as canonical authoring or
golden-reference material unless there is a strong reason to drop them.

The authoritative schema for this envelope is `schema/drp_fixture.schema.json`.
If this prose and that schema diverge, this prose defines the intended contract and the schema should
be updated promptly.


## Top-Level Fixture Object

Each fixture is a single JSON object.

Required normative fields:

1. `name`
2. `version`
3. `commands`
4. `expected`

Optional normative fields:

1. `capabilities`
2. `tags`

Optional non-normative human metadata fields:

1. `description`
2. `reason`
3. `fix`
4. `notes`


## Normative Fields

### `name`

Stable fixture identifier as a string.


### `version`

Protocol version object with:

1. required `major`
2. required `minor`
3. optional `patch`

This mirrors `VERSIONING.md`.


### `capabilities`

Optional capability assumptions for the fixture.

This field is normative when present.
If a fixture depends on a capability gate, the gate must be expressed here rather than buried in
free-form notes.

For the first executable `2.0` corpus, the active fixture capability shape is defined in
`CAPABILITIES.md`.


### `tags`

Optional string list for categorization such as `negative`, `state`, `lifetime`, or `range`.


### `commands`

Ordered list of DRP2 command objects.

Rules:

1. command indexing is zero-based,
2. every positive active fixture stream must start with `HelloRenderer` followed by
   `RendererHelloReply`,
3. semantic negatives that intentionally test handshake establishment may violate that happy-path
   prefix and should continue to fail in semantic validation rather than fixture-envelope validation,
4. for positive fixtures and semantic negatives, command objects must use the active DRP2 `2.0`
   field names,
5. schema-negative fixtures may intentionally include command objects that fail the active command
   schema,
6. deferred command names and deferred schema shapes must not appear in the first `2.0` corpus,
7. command semantics are defined by `COMMANDS.md`, `LIFETIMES.md`, and the active schemas.


### `expected`

Authoritative expected outcome for the fixture.

Every fixture must declare:

1. `expected.outcome`

Negative fixtures must also declare:

1. `expected.phase`
2. `expected.code`
3. optional `expected.command_index`

Positive fixtures may additionally declare:

1. other deterministic success outputs once the positive corpus is frozen


## Expected Outcome Semantics

### Negative Fixture

Use:

1. `expected.outcome = "error"`
2. `expected.phase` as one of:
   - `schema_validation`
   - `semantic_validation`
   - `capability_validation`
3. `expected.code` as a symbolic error code from `ERRORS.md`
4. `expected.command_index` as the zero-based index of the primary failing command when the failure
   is attributable to a single command

Rules:

1. a negative fixture should express exactly one intended primary failure,
2. the expected phase should be the earliest deterministic phase at which the failure is detectable,
3. if multiple errors could be reported, the fixture should be written so one primary failure is
   clearly dominant,
4. if `expected.phase` is `schema_validation`, the fixture may intentionally contain command objects
   that do not satisfy the active DRP2 command schema.


### Positive Fixture

Use:

1. `expected.outcome = "success"`

Positive fixtures currently express only pass/fail at the active fixture-contract level.
Deterministic success outputs such as readback hashes remain deferred until the runner implements
them explicitly.


## Human Metadata

The following fields are allowed for readability but are non-normative:

### `description`

One-sentence summary of what the fixture is testing.


### `reason`

Plain-language explanation of why the stream should pass or fail.
This should cite the violated or satisfied rule at a human level, not restate the entire spec.


### `fix`

For negative fixtures, the shortest valid repair to the stream.


### `notes`

Optional extra context for authors and reviewers.


## Naming

Use JSON filenames.

Recommended naming:

1. positive fixtures: `render_<story>.json`, `compute_<story>.json`
2. negative fixtures: `invalid_<category>_<case>.json`

Examples:

1. `render_hello_triangle.json`
2. `render_indexed_basic.json`
3. `invalid_duplicate_buffer_id.json`
4. `invalid_draw_outside_render_pass.json`


## Directory Layout

The active corpus is split into:

1. `positive/`
2. `negative/`
3. `negative_schema/`

Negative fixtures lock the validation boundary for the active command surface. Positive fixtures
lock canonical valid traces and pressure cases.


## Authoring Rules

1. prefer small fixtures over broad scenario bundles,
2. keep each negative fixture focused on one primary failure,
3. keep human metadata concise and accurate,
4. do not encode normative behavior only in `description`, `reason`, `fix`, or `notes`,
5. when a fixture exercises lifetime or state rules, make the expected error code consistent with
   `ERRORS.md` and `LIFETIMES.md`,
6. keep schema-negative fixtures envelope-valid even when their `commands` payload is intentionally
   invalid against the active command schema.
