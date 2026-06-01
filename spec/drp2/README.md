# DRP2 Spec

This directory defines the future Datoviz Rendering Protocol v2.

The goal is to freeze a small, backend-agnostic renderer contract that can support:

1. a native runtime with a Vulkan backend,
2. a browser runtime over WebGPU,
3. future higher-level scene layers that emit DRP2 rather than depending on backend internals.


## Status

- Status: **active 2.0 contract plus native implementation slice** — command surface, schemas,
  fixture corpus, C command streams, validation, and the vklite runtime are stable enough for
  incremental scene work
- Conformance: Level 1 (validation) mechanically verified; Level 2 (execution) prose commitment; Level 3 (output) deferred to `2.1`
- Build integration: fixture validation available through the Python runner; focused C tests available
  through `just test drp2`
- Implementation priority: harden the active scene -> DRP2 -> runtime slice and keep fixtures aligned
  with implementation behavior; use a narrow WebGPU feasibility lane to pressure the contract before
  the visual surface grows too large


## Start Here

1. [AUTHORITY.md](AUTHORITY.md): conflict resolution and source-of-truth order.
2. [READING_ORDER.md](READING_ORDER.md): recommended review order.
3. [LAYER1.md](LAYER1.md): primary human-readable contract overview.


## Document Index

- [LAYER1.md](LAYER1.md): primary human-readable contract
- [AUTHORITY.md](AUTHORITY.md): source-of-truth and conflict-resolution rules
- [READING_ORDER.md](READING_ORDER.md): review order for DRP2 contributors
- [COMMANDS.md](COMMANDS.md): active `2.0` command surface
- [LIFETIMES.md](LIFETIMES.md): object lifetime and encoder/pass state rules
- [PACKETS.md](PACKETS.md): binary packet, payload arena, and setup/update/frame transport
- [ERRORS.md](ERRORS.md): validation and error model
- [CAPABILITIES.md](CAPABILITIES.md): feature and format capability reporting
- [CONFORMANCE.md](CONFORMANCE.md): conformance levels and requirements
- [VERSIONING.md](VERSIONING.md): compatibility and contract-evolution rules
- [GLOSSARY.md](GLOSSARY.md): fixed terminology
- [schema/README.md](schema/README.md): machine-readable schema material
- [fixtures/README.md](fixtures/README.md): canonical conformance traces
- [recording/README.md](recording/README.md): DRP2/DVZR recording and replay design notes
- [roadmap/README.md](roadmap/README.md): long-horizon WebGPU and native/browser parity roadmap


## Validation

The current DRP2 contract can be checked with the fixture runner:

```bash
just drp2-fixtures
```

The current spec-level validation entrypoint is:

```bash
just spec-check
```

Direct invocation is also available:

```bash
python3 tools/drp2_fixture_runner.py
python3 tools/webgpu_fixture_preflight.py
```

Useful focused examples:

```bash
python3 tools/drp2_fixture_runner.py spec/drp2/fixtures/negative_schema
python3 tools/drp2_fixture_runner.py --tag queue
python3 tools/drp2_fixture_runner.py --json spec/drp2/fixtures/positive/write_buffer_basic.json
```

The runner contract lives in [fixtures/RUNNER.md](fixtures/RUNNER.md).


## Scope Discipline

DRP2 should start narrow.

The first stable contract should cover only the minimum needed for a practical renderer slice:

1. resource creation and update,
2. shader-module and pipeline creation,
3. command encoding,
4. render passes and draw calls,
5. copy operations,
6. queue submission,
7. strict validation and capability reporting.

Scene concerns, browser transport, profiling, native interop, and advanced memory policy are all
important, but they should pressure-test the contract rather than bloat the first contract freeze.
The currently active pressure points are native depth-enabled 3D scenes, point/image readbacks for
pick/probe requests, and browser/WebGPU portability of the existing resource/pipeline/pass subset.


## `2.0` Decisions Already Taken

1. explicit destroy commands are part of `2.0`,
2. compute remains mandatory in the first frozen contract,
3. handshake and version-negotiation commands are now part of the executable fixture core,
4. `HelloRenderer` is required as the first command of every fixture stream in the active corpus,
5. prototype C API sketches are intentionally excluded until the contract is tighter.
