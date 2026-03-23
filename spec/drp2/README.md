# DRP2 Spec

This directory defines the future Datoviz Rendering Protocol v2.

The goal is to freeze a small, backend-agnostic renderer contract that can support:

1. a native runtime with a Vulkan backend,
2. a browser runtime over WebGPU,
3. future higher-level scene layers that emit DRP2 rather than depending on backend internals.


## Status

- Status: active contract-definition with executable conformance fixtures
- Build integration: fixture validation available through the Python runner
- Implementation priority: after the current `vk`/`vklite` boundary cleanup is sufficiently stable


## Documents

- `LAYER1.md`: primary human-readable contract
- `COMMANDS.md`: frozen `2.0` command surface
- `LIFETIMES.md`: object lifetime and encoder/pass state rules
- `ERRORS.md`: validation and error model
- `CAPABILITIES.md`: feature and format capability reporting
- `VERSIONING.md`: compatibility and contract-evolution rules
- `GLOSSARY.md`: fixed terminology
- `schema/`: machine-readable schema material
- `fixtures/`: canonical conformance traces


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
```

Useful focused examples:

```bash
python3 tools/drp2_fixture_runner.py spec/drp2/fixtures/negative_schema
python3 tools/drp2_fixture_runner.py --tag queue
python3 tools/drp2_fixture_runner.py --json spec/drp2/fixtures/positive/write_buffer_basic.json
```

The runner contract lives in `fixtures/RUNNER.md`.


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


## `2.0` Decisions Already Taken

1. explicit destroy commands are part of `2.0`,
2. compute remains mandatory in the first frozen contract,
3. prototype C API sketches are intentionally excluded until the contract is tighter.
