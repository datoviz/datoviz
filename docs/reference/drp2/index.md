# DRP2

DRP2 is the Datoviz Rendering Protocol v2: the backend-agnostic command-stream boundary between
scene semantics and runtime execution.

```text
scene frame plans -> DRP2 command streams -> vklite/WebGPU runtime
```

DRP2 is an advanced/unstable contributor and backend-author surface. Ordinary users should usually
work through scene, visual, app, and example APIs.

## What DRP2 Owns

| Area | Role |
| --- | --- |
| Command stream | Typed backend-neutral setup, update, frame, copy, render, compute, and submit commands. |
| Validation | Schema and semantic checks for command order, object lifetimes, pass state, formats, capabilities, and malformed streams. |
| Fixtures | Canonical positive and negative traces used by native and WebGPU validation. |
| Packets | Binary setup/update/frame packet transport for the WASM/WebGPU runtime path. |
| Recording/replay | DVZR/debug workflows for inspecting or replaying command streams. |
| Capabilities | Runtime feature/format support used by scene planning and backend diagnostics. |

DRP2 does not own scene semantics, visual-family APIs, panel layout, controller behavior, or user
interaction policy.

## Active Contract

| Topic | Status |
| --- | --- |
| Native C command streams | Active implementation slice. |
| Fixture validation | Active through `just drp2-fixtures` and related WebGPU preflight tools. |
| vklite runtime execution | Active for the runtime subset used by scene/app paths. |
| WebGPU runner | Experimental but active for the committed fixture subset. |
| Compute commands and `ResourceBarrier` | Active narrow compute-to-render slice. |
| Full output conformance | Deferred beyond the initial v0.4 contract. |

## Validation

Use these checks for DRP2/spec work:

```sh
just drp2-fixtures
just spec-check
just test drp2
```

Focused direct tools:

```sh
python3 tools/drp2_fixture_runner.py
python3 tools/webgpu_fixture_preflight.py
```

## Source Of Truth

Durable DRP2 authority lives under `spec/drp2/`:

| Spec | Purpose |
| --- | --- |
| `spec/drp2/AUTHORITY.md` | Conflict resolution and source-of-truth order. |
| `spec/drp2/LAYER1.md` | Human-readable contract overview. |
| `spec/drp2/COMMANDS.md` | Active command surface. |
| `spec/drp2/LIFETIMES.md` | Object lifetime and encoder/pass state rules. |
| `spec/drp2/PACKETS.md` | Binary packet and payload arena transport. |
| `spec/drp2/ERRORS.md` | Validation and error model. |
| `spec/drp2/CAPABILITIES.md` | Capability and format reporting. |
| `spec/drp2/CONFORMANCE.md` | Conformance levels and requirements. |
| `spec/drp2/fixtures/` | Canonical fixture corpus. |

## See Also

- [WebGPU subset](../webgpu-subset.md)
- [Compute and graphics](../compute-graphics.md)
- [Record and replay frame streams](../../how-to/record-replay.md)
- [Adding a DRP2 command](../../contributors/adding-a-drp2-command.md)
