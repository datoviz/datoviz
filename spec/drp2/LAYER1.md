# DRP2 Layer 1

Human-readable contract for the future Datoviz Rendering Protocol v2.

Status: active reduced contract
Scope: intentionally reduced first contract


## Purpose

DRP2 is the backend-agnostic rendering contract that future Datoviz runtimes should execute.

It exists to let higher-level producers emit one logical GPU command stream that can be consumed by:

1. a native runtime with a Vulkan backend,
2. a browser runtime over WebGPU,
3. future tooling for replay, validation, capture, and testing.


## Non-Goals

This first contract freeze is not trying to define:

1. the full future scene API,
2. native interop escape hatches,
3. profiling and benchmarking APIs,
4. a complete binary transport format,
5. advanced memory-management policy.

Those topics matter, but they should remain outside the initial DRP2 contract freeze unless they are
strictly required by the minimal renderer slice.


## Design Principles

1. Backend-agnostic public contract.
2. WebGPU-shaped semantics where practical.
3. No `Vk*` types or Vulkan constants in public DRP2 definitions.
4. Deterministic validation and replay at the contract level.
5. Narrow first version, expand later.


## Object Model

The first DRP2 contract defines logical objects only.
Backends remain free to implement them with different physical strategies.

Active object kinds:

1. buffer
2. texture
3. bind group layout
4. bind group
5. render pipeline
6. compute pipeline
7. command encoder
8. render pass encoder
9. compute pass encoder

Each object is addressed by an explicit logical id chosen by the producer.


## Command Categories

The reduced `2.0` contract covers only these categories:

1. resource creation and destruction,
2. resource upload and copy,
3. lightweight pipeline creation,
4. command encoder lifecycle,
5. render pass lifecycle,
6. compute pass lifecycle,
7. draw and dispatch,
8. queue submission.


## Frozen `2.0` Decisions

These decisions are already taken for the first contract freeze:

1. explicit destroy commands are part of `2.0`,
2. compute is mandatory in `2.0`,
3. prototype C API sketches are intentionally excluded until the contract is tighter.


## Required First-Slice Commands

The frozen `2.0` command surface is defined in `COMMANDS.md`.

At a minimum, `2.0` includes:

1. resource lifecycle commands,
2. binding and pipeline lifecycle commands,
3. render-pass commands,
4. compute-pass commands,
5. draw and dispatch commands,
6. copy commands,
7. queue submission.


## Execution Semantics

1. Commands are immutable once emitted.
2. Commands are consumed in order.
3. Validation is part of the contract, not an optional debug feature.
4. Implicit synchronization should follow WebGPU-like semantics in the first version.
5. Explicit backend-specific synchronization is deferred unless it becomes necessary for the minimal
   renderer slice.


## Resource Rules

1. Every resource has an explicit logical id.
2. Resource usage must be declared at creation time.
3. Upload and copy commands must obey documented range, layout, and alignment rules.
4. Texture formats and shader formats are validated against the capability model.
5. The public contract describes logical resources, not allocation strategy.


## Shader Rules

1. Shader-module objects are deferred from the active executable `2.0` surface.
2. WGSL remains the intended default contract-level shader language once shader ingestion is promoted.
3. Native-only ingestion paths such as SPIR-V may exist behind explicit capability flags once shader
   ingestion is promoted.
4. The active `2.0` surface validates only lightweight pipeline metadata such as bind-group-layout
   expectations and required vertex-buffer slots.


## Pass Rules

Render and compute passes are explicit encoder scopes.

Rules:

1. draw commands are valid only inside a render pass,
2. dispatch commands are valid only inside a compute pass,
3. attachments, load/store operations, and pipeline state must be validated before execution,
4. pass compatibility is a contract-level concern, not only a backend detail.


## Validation

Every runtime should implement the same logical validation model.

At minimum, validation covers:

1. object existence,
2. object type compatibility,
3. lifetime and state transitions,
4. command ordering,
5. pass scope correctness,
6. binding compatibility,
7. resource range and layout checks,
8. capability gating,
9. version compatibility.

Detailed symbolic codes live in `ERRORS.md`.
Detailed lifetime and state rules live in `LIFETIMES.md`.


## Capability Model

DRP2 must have an explicit capability report used before feature-dependent command streams are emitted.

At minimum the capability model must cover:

1. supported protocol versions,
2. shader language support,
3. texture format support,
4. sample count support,
5. FP64 support,
6. key size and binding limits.

Detailed rules live in `CAPABILITIES.md`.


## Versioning

Every stream and handshake message declares a DRP2 protocol version object.
The schema, fixture set, and human-readable Layer 1 contract for a given version must be kept in
lockstep.

Detailed rules live in `VERSIONING.md`.
Terminology is fixed in `GLOSSARY.md`.


## Conformance

The contract is not ready until it has:

1. machine-readable schemas,
2. canonical fixtures,
3. negative fixtures for validation failures,
4. native and browser replay expectations.

The command surface for `2.0` is frozen in `COMMANDS.md`.

The active fixture core does not yet exercise handshake/version negotiation; that slice remains
specified but not yet part of first conformance.


## Pressure Tests From Future Scene Work

The first contract freeze should be checked against at least these producer stories:

1. static geometry in a panel,
2. dynamic buffer updates across frames,
3. texture upload and sampling,
4. picking-oriented render-to-texture plus readback,
5. one compute-assisted data path.

If DRP2 cannot express those cleanly without backend leakage, the contract is not ready.
