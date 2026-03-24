# DRP2 Versioning

This document defines how the DRP2 contract evolves.


## Principles

1. Version the contract explicitly from day one.
2. Prefer additive evolution.
3. Reject ambiguous compatibility rules.
4. Keep schema, fixtures, and human-readable contract in lockstep.


## Version Shape

Use semantic fields at the protocol level:

1. `major`
2. `minor`
3. optional `patch` for documentation and fixtures

Compatibility rules:

1. major changes may break compatibility,
2. minor changes may add commands, fields, or capabilities in a backward-compatible way,
3. patch changes must not change semantics.


## Producer and Runtime Rules

1. Every command stream declares its protocol version.
2. Handshake commands use the same `{major, minor, patch?}` version object as fixtures.
3. Runtimes declare the versions they accept.
4. Unknown major versions fail immediately.
5. Unknown optional fields in the same major version may be ignored only when the schema says so.
6. The schema and fixture set for a given version must be frozen together.


## Freeze Strategy

Before implementation work starts in earnest:

1. freeze a minimal `2.0` command set,
2. freeze the validation error codes used by first conformance tests,
3. freeze the first fixture corpus,
4. defer large feature additions to `2.1+` rather than churning `2.0`.
