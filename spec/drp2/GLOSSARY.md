# DRP2 Glossary

This document fixes the core terms used by the DRP2 spec.


## Terms

### Producer

The software that emits a DRP2 command stream.

Examples:

1. a future Datoviz scene layer,
2. a replay tool,
3. a test fixture generator.


### Runtime

The implementation layer that decodes, validates, and executes DRP2 commands against a backend.


### Backend

The platform-specific graphics implementation used by a runtime.

Examples:

1. Vulkan
2. browser WebGPU


### Logical Object

An object described by the DRP2 contract and identified by a producer-chosen logical id.

Examples:

1. buffer
2. texture
3. bind group
4. render pipeline

Logical objects are contract-level entities, not promises about backend allocation strategy.


### Command Stream

An ordered sequence of immutable DRP2 commands.


### Capability

A structured runtime-reported fact about supported protocol versions, formats, limits, or optional
features.


### Conformance Fixture

A versioned, backend-agnostic test artifact that defines an input stream and an expected validation
or rendering outcome.


### Validation

The contract-level process that checks decode, schema, semantic, and capability correctness before
or during execution.
