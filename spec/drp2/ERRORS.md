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
