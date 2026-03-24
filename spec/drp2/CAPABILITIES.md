# DRP2 Capabilities

This document defines the minimum capability model needed by DRP2.


## Goals

1. Keep the public contract backend-agnostic.
2. Make optional features explicit and testable.
3. Avoid embedding Vulkan-specific or browser-specific constants in the public DRP2 surface.


## Capability Categories

The runtime should expose a structured capability record covering:

1. protocol version support
2. shader language support
3. texture format support
4. sample count support
5. depth/stencil support
6. compute support
7. limits
8. determinism-related guarantees
9. optional native escape hatches outside the core DRP2 contract


## Minimum Fields

The first capability schema should include at least:

1. `max_bind_groups`
2. `max_color_attachments`
3. `max_buffer_size`
4. `max_uniform_buffer_binding_size`
5. `max_storage_buffer_binding_size`
6. `max_texture_dimension_1d`
7. `max_texture_dimension_2d`
8. `max_texture_dimension_3d`
9. `max_texture_array_layers`
10. `supported_texture_formats`
11. `supported_sample_counts`
12. `supports_compute`
13. `supports_timestamp_queries`
14. `supports_fp64`
15. `supported_shader_formats`


## Fixture Capability Shape

When a fixture declares `capabilities`, the first active `2.0` capability object should use this
shape:

1. `max_buffer_size`
2. `max_texture_dimension_1d`
3. `max_texture_dimension_2d`
4. `max_texture_dimension_3d`
5. `supported_texture_formats`
6. `supported_sample_counts`
7. `supports_compute`

The omitted fields from the broader future capability model remain planned but are not yet consumed
by the first executable fixture corpus.

Active `2.0` note:

1. dynamic buffer offsets do not currently introduce a separate capability field,
2. the active contract deliberately does not model backend-specific dynamic-offset alignment limits,
3. if a future runtime needs explicit dynamic-offset alignment reporting, that should be added as a
   later capability slice rather than retrofitted implicitly into the current fixture corpus.


## Active `2.0` Command Gates

The first active `2.0` capability validation rules are:

1. `CreateBuffer.size` must not exceed `max_buffer_size`,
2. `CreateTexture.format` must be listed in `supported_texture_formats`,
3. `CreateTexture.sample_count` must be listed in `supported_sample_counts`,
4. `CreateTexture` dimensions must not exceed the corresponding `max_texture_dimension_*` field for
   the chosen texture dimension,
5. `BeginComputePass` and `DispatchWorkgroups` require `supports_compute = true`.

Capability failures should occur during `capability_validation` after schema and semantic validation
have succeeded.


## Feature Gating Rules

1. If a command needs a capability, that dependency must be explicit in the contract.
2. Unsupported features fail during capability validation, before backend submission.
3. Capability reports are declarative; they must not expose backend handle types.
4. WGSL should be the default shader language for the contract.
5. Native-only ingestion paths such as SPIR-V belong behind explicit capability flags.


## Error Mapping

The first capability-validation mapping should be:

1. use `DRP2_ERR_UNSUPPORTED_CAPABILITY` when the runtime explicitly reports lack of support for a
   requested capability,
2. use `DRP2_ERR_FEATURE_REQUIRED` when the command stream requires a feature the runtime reports as
   unavailable and the fixture is explicitly testing that required feature boundary,
3. for the current active fixture corpus, prefer `DRP2_ERR_UNSUPPORTED_CAPABILITY`.


## FP64

FP64 should be treated as optional-but-reportable at the contract level.

Rules:

1. The capability report must state whether FP64 is supported.
2. Pipelines or shader modules that require FP64 must declare that requirement.
3. Producers must be able to select a fallback path when FP64 is unavailable.


## Determinism

The capability model should distinguish:

1. best-effort execution,
2. deterministic rendering and readback where guaranteed,
3. deterministic compute/reduction modes where explicitly supported.

Determinism should be requested by contract-level policy, not inferred from a backend.
