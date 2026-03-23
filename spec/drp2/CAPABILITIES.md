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


## Feature Gating Rules

1. If a command needs a capability, that dependency must be explicit in the contract.
2. Unsupported features fail during capability validation, before backend submission.
3. Capability reports are declarative; they must not expose backend handle types.
4. WGSL should be the default shader language for the contract.
5. Native-only ingestion paths such as SPIR-V belong behind explicit capability flags.


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
