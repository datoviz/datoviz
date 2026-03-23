# DRP2 Fixture Format

This document defines the intended shape of the first DRP2 conformance fixtures.


## Goals

1. backend-agnostic input representation,
2. versioned fixture metadata,
3. support for both positive and negative cases,
4. support for deterministic replay expectations.


## Minimum Fixture Fields

Each fixture should define at least:

1. fixture name
2. DRP2 protocol version
3. capability assumptions
4. command stream payload
5. expected outcome


## Expected Outcome Kinds

### Positive Fixture

Must define at least one of:

1. expected validation success
2. expected readback hash
3. expected metadata about produced resources


### Negative Fixture

Must define:

1. expected failure phase
2. expected symbolic error code
3. optional expected command index


## Naming

Recommended initial naming:

1. `render_hello_triangle`
2. `render_indexed_basic`
3. `render_texture_sample`
4. `compute_basic_dispatch`
5. `invalid_duplicate_id`
6. `invalid_draw_outside_pass`


## Representation

The first fixtures may use JSON for readability.
If a binary encoding is introduced later, the JSON fixtures should remain as canonical authoring or
golden-reference material unless there is a strong reason to drop them.
