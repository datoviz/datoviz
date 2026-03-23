# DRP2 Spec

This directory defines the future Datoviz Rendering Protocol v2.

The goal is to freeze a small, backend-agnostic renderer contract that can support:

1. a native runtime with a Vulkan backend,
2. a browser runtime over WebGPU,
3. future higher-level scene layers that emit DRP2 rather than depending on backend internals.


## Status

- Status: planning and contract-definition only
- Build integration: none yet
- Implementation priority: after the current `vk`/`vklite` boundary cleanup is sufficiently stable


## Documents

- `LAYER1.md`: primary human-readable contract
- `ERRORS.md`: validation and error model
- `CAPABILITIES.md`: feature and format capability reporting
- `VERSIONING.md`: compatibility and contract-evolution rules
- `schema/`: machine-readable schema material
- `fixtures/`: canonical conformance traces
- `prototypes/`: non-authoritative exploratory C API sketches


## Scope Discipline

DRP2 should start narrow.

The first stable contract should cover only the minimum needed for a practical renderer slice:

1. resource creation and update,
2. shader and pipeline creation,
3. command encoding,
4. render passes and draw calls,
5. copy operations,
6. queue submission,
7. strict validation and capability reporting.

Scene concerns, browser transport, profiling, native interop, and advanced memory policy are all
important, but they should pressure-test the contract rather than bloat the first contract freeze.
