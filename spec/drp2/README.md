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


## Recommended Reading Order

Read the DRP2 spec in this order during review.

### 1. Contract overview and terminology

1. [LAYER1.md](LAYER1.md) — primary human-readable contract overview
2. [GLOSSARY.md](GLOSSARY.md) — fixed protocol terminology used by the rest of the spec
3. [VERSIONING.md](VERSIONING.md) — compatibility and evolution rules for the contract

### 2. Normative protocol surface

4. [COMMANDS.md](COMMANDS.md) — active `2.0` command surface and command semantics
5. [LIFETIMES.md](LIFETIMES.md) — object lifetime and encoder/pass state rules
6. [ERRORS.md](ERRORS.md) — validation and error model
7. [CAPABILITIES.md](CAPABILITIES.md) — feature and format capability reporting
8. [CONFORMANCE.md](CONFORMANCE.md) — conformance levels and requirements for `2.0`
9. [WEBGPU_ROADMAP.md](WEBGPU_ROADMAP.md) — strategic native/browser parity roadmap

### 3. Machine-readable review material

9. [schema/README.md](schema/README.md) — schema authority, active files, and maintenance rules
10. [schema/drp_command.json](schema/drp_command.json) — root union for active commands
11. [schema/commands/](schema/commands/) — per-command schema files
12. [schema/common/](schema/common/) — shared enums and common value types
13. [schema/DEFERRED.md](schema/DEFERRED.md) — explicitly deferred schema inventory

### 4. Executable conformance material

14. [fixtures/README.md](fixtures/README.md) — fixture corpus overview
15. [fixtures/FORMAT.md](fixtures/FORMAT.md) — fixture file format
16. [fixtures/RUNNER.md](fixtures/RUNNER.md) — fixture runner behavior and usage
17. [fixtures/positive/](fixtures/positive/) — canonical valid traces
18. [fixtures/negative/](fixtures/negative/) — canonical invalid semantic traces
19. [fixtures/negative_schema/](fixtures/negative_schema/) — canonical invalid schema traces


## Document Index

- [LAYER1.md](LAYER1.md): primary human-readable contract
- [COMMANDS.md](COMMANDS.md): active `2.0` command surface
- [LIFETIMES.md](LIFETIMES.md): object lifetime and encoder/pass state rules
- [ERRORS.md](ERRORS.md): validation and error model
- [CAPABILITIES.md](CAPABILITIES.md): feature and format capability reporting
- [CONFORMANCE.md](CONFORMANCE.md): conformance levels and requirements
- [VERSIONING.md](VERSIONING.md): compatibility and contract-evolution rules
- [GLOSSARY.md](GLOSSARY.md): fixed terminology
- [schema/README.md](schema/README.md): machine-readable schema material
- [fixtures/README.md](fixtures/README.md): canonical conformance traces
- [WEBGPU_ROADMAP.md](WEBGPU_ROADMAP.md): long-horizon WebGPU and native/browser parity roadmap


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
