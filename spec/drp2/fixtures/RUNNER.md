# DRP2 Fixture Runner Contract

This document defines the minimal runner behavior for the first DRP2 `2.0` fixture corpus.

It is a contract for validation and replay harnesses, not an implementation guide for any specific
language or backend.


## Goals

1. discover the same fixture set deterministically,
2. classify outcomes consistently across runtimes,
3. make first-failure reporting uniform,
4. keep schema negatives, semantic negatives, and positive fixtures in one reusable corpus.


## Fixture Discovery

The runner should discover fixtures under:

1. `fixtures/positive/`
2. `fixtures/negative/`
3. `fixtures/negative_schema/`

Rules:

1. only `.json` files are fixtures,
2. discovery order should be lexical by relative path,
3. a runner may filter by path, directory, fixture `name`, or `tags`,
4. filtering must not change the meaning of an individual fixture.


## Fixture Classes

The runner must classify fixtures by `expected.outcome` and `expected.phase`.

### Positive Fixtures

Definition:

1. `expected.outcome = "success"`

Required runner behavior:

1. validate the fixture envelope,
2. validate every command object against the active DRP command schema,
3. run decode, semantic validation, capability checks, and execution only as far as the fixture
   contract currently requires,
4. report success only if no earlier phase fails.


### Semantic Negative Fixtures

Definition:

1. `expected.outcome = "error"`
2. `expected.phase != "schema_validation"`

Required runner behavior:

1. validate the fixture envelope,
2. validate every command object against the active DRP command schema,
3. continue into the expected validation or execution phase,
4. report success only if the actual primary failure matches the fixture expectation.


### Schema-Negative Fixtures

Definition:

1. `expected.outcome = "error"`
2. `expected.phase = "schema_validation"`

Required runner behavior:

1. validate the fixture envelope,
2. allow the embedded `commands` payload to be invalid against the active DRP command schema,
3. stop once schema validation of the command payload fails,
4. do not continue to semantic validation or execution.


## Required Execution Pipeline

For each fixture, the runner should model these phases in order:

1. fixture envelope validation,
2. command decode,
3. command schema validation,
4. semantic validation,
5. capability validation,
6. execution

Rules:

1. the earliest deterministic failure wins,
2. once a fixture has failed in one phase, later phases must not replace that result,
3. a runner may internally combine decode and schema setup steps, but the reported phase must still
   map to the contract phase names,
4. fixture-envelope failure is a runner or corpus-authoring problem, not a DRP command-stream result.


## First-Failure Semantics

The runner must report one primary result per fixture.

Rules:

1. the primary result is the earliest deterministic failing phase,
2. if a failing command can be identified, report its zero-based `command_index`,
3. if the failure is attributable to multiple later commands, only the first failing command counts,
4. backend-native secondary diagnostics may be recorded, but they must not replace the contract-level
   primary result,
5. a fixture passes only if the primary result exactly matches the expected outcome.


## Result Matching

### Positive Fixture Match

A positive fixture matches only if:

1. no failure occurs in any earlier phase,
2. any declared deterministic success outputs also match.


### Negative Fixture Match

A negative fixture matches only if:

1. `actual.outcome = "error"`,
2. `actual.phase = expected.phase`,
3. `actual.code = expected.code`,
4. if `expected.command_index` is present, `actual.command_index` matches it exactly.


## Required Result Shape

Every runner should be able to emit a structured result record with at least:

1. fixture path
2. fixture name
3. actual outcome
4. actual phase
5. actual code, if any
6. actual command index, if known
7. pass/fail boolean
8. optional short message

For failed expectation matches, runners should also emit:

1. expected outcome
2. expected phase
3. expected code
4. expected command index, if present


## Reporting Conventions

1. `command_index` is zero-based everywhere,
2. omitted `command_index` means the failure is not attributable to a single command or the fixture
   intentionally does not require it,
3. runners should display both the fixture path and `name` because names are stable but paths help
   debugging,
4. schema-negative fixtures should be reported distinctly enough that they are not confused with
   semantic negatives.


## Minimal Runner Summary

At minimum, a batch runner should report:

1. total fixtures discovered,
2. fixtures run,
3. fixtures passed,
4. fixtures failed,
5. failing fixture paths with mismatch details.


## Non-Goals For The First Runner Contract

This document does not yet define:

1. snapshot image formats,
2. readback hashing algorithms,
3. capability negotiation file formats,
4. parallel execution rules,
5. sharding or caching behavior.

Those can be added later without changing the basic pass/fail contract for the first corpus.
