# DRP2 Error Model

This document defines the shape of validation and execution errors for DRP2.


## Goals

1. Errors must be backend-agnostic at the contract level.
2. Producers must get actionable diagnostics without needing Vulkan or WebGPU knowledge.
3. Native and browser runtimes must report the same logical error categories for the same invalid
   command stream.


## Error Classes

Use stable symbolic codes, not backend strings, as the primary contract.

1. `DRP2_ERR_UNKNOWN_COMMAND`
2. `DRP2_ERR_UNSUPPORTED_VERSION`
3. `DRP2_ERR_UNSUPPORTED_CAPABILITY`
4. `DRP2_ERR_INVALID_ID`
5. `DRP2_ERR_DUPLICATE_ID`
6. `DRP2_ERR_WRONG_OBJECT_TYPE`
7. `DRP2_ERR_DESTROYED_OBJECT`
8. `DRP2_ERR_INVALID_STATE`
9. `DRP2_ERR_INVALID_ARGUMENT`
10. `DRP2_ERR_OUT_OF_RANGE`
11. `DRP2_ERR_ALIGNMENT`
12. `DRP2_ERR_LAYOUT`
13. `DRP2_ERR_USAGE`
14. `DRP2_ERR_PASS_MISMATCH`
15. `DRP2_ERR_PIPELINE_MISMATCH`
16. `DRP2_ERR_BINDING_MISMATCH`
17. `DRP2_ERR_FORMAT_MISMATCH`
18. `DRP2_ERR_FEATURE_REQUIRED`
19. `DRP2_ERR_OOM`
20. `DRP2_ERR_INTERNAL`


## Reporting Shape

Every runtime should be able to map a failure to a structured record with at least:

1. code
2. severity
3. command index
4. object id, if relevant
5. short message
6. optional backend detail string

Backend detail may be richer, but the stable contract is the symbolic code and its semantics.


## Validation Phases

Errors should be attributable to one of these phases:

1. decode
2. schema validation
3. semantic validation
4. capability validation
5. execution

The same logical stream should fail as early as possible.
If a problem is detectable during decode or semantic validation, runtimes should not defer it until
backend execution.


## Contract Rules

1. Invalid streams are rejected deterministically.
2. Validation failures are not recoverable within the same command.
3. An invalid command does not permit partially-committed object state.
4. Backend-native validation messages are supplementary and non-normative.
5. Unsupported capabilities must fail with a capability-oriented error, not a generic internal error.


## Code Selection Rules

Use the most specific error code that matches the contract-visible failure.

### Identity And Type

1. use `DRP2_ERR_INVALID_ID` when the referenced id was never created,
2. use `DRP2_ERR_DESTROYED_OBJECT` when the referenced id existed earlier in the same stream but has
   already been destroyed,
3. use `DRP2_ERR_DUPLICATE_ID` when a creation command reuses an already-used id,
4. use `DRP2_ERR_WRONG_OBJECT_TYPE` when the id exists and is live but belongs to the wrong object
   kind.


### State Versus Usage

1. use `DRP2_ERR_INVALID_STATE` when the failure is about recorder scope or sequencing,
2. use `DRP2_ERR_USAGE` when the failure is about whether a live resource may legally be used or
   destroyed in the current contract state,
3. prefer `DRP2_ERR_INVALID_STATE` for examples such as:
   - draw outside a render pass
   - draw or dispatch without a pipeline bound in the current pass
   - dispatch outside a compute pass
   - finishing an encoder with an open pass
   - resubmitting a command buffer that was already submitted earlier in the stream
   - issuing a copy command inside a pass
4. prefer `DRP2_ERR_USAGE` for examples such as:
   - destroying a resource still referenced by recorded or submitted work
   - using a resource in a way forbidden by its declared creation usage


### Range, Layout, And Alignment

1. use `DRP2_ERR_OUT_OF_RANGE` when numeric bounds are exceeded after the command shape is otherwise
   valid,
2. use `DRP2_ERR_LAYOUT` when the memory-layout description itself is invalid or inconsistent,
3. use `DRP2_ERR_ALIGNMENT` when the only failing condition is an alignment requirement,
4. prefer `DRP2_ERR_OUT_OF_RANGE` for examples such as:
   - `offset + size` exceeding buffer bounds
   - a texture write region exceeding the destination subresource extent
5. prefer `DRP2_ERR_LAYOUT` for examples such as:
   - impossible or self-contradictory row-stride / image-stride declarations
   - layout metadata incompatible with the transfer shape even when the destination range exists


### Phase Preference

1. schema-shape failures should report a schema-validation phase before semantic codes are considered,
2. semantic code selection applies only after the command object has passed schema validation,
3. capability-dependent failures should prefer `DRP2_ERR_UNSUPPORTED_CAPABILITY` or
   `DRP2_ERR_FEATURE_REQUIRED` over generic usage or internal errors.


## First Conformance Set

The first fixture set should include negatives for:

1. duplicate object creation
2. using unknown ids
3. wrong object type in a command
4. draw outside a render pass
5. incompatible pipeline in a pass
6. invalid binding layout
7. unsupported format or feature
8. invalid copy ranges or alignment
9. schema-shape failures such as missing discriminators or missing required fields
