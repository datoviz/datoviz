# DRP2

DRP2 is Datoviz Rendering Protocol v2: the backend-neutral command stream between scene planning
and runtime execution.

```text
scene frame artifact -> DRP2 packets -> runtime
```

Status: advanced/unstable. Ordinary visualization code should use scene, visual, app, and example
APIs.

## What DRP2 Owns

| Area | Role |
| --- | --- |
| Commands | Typed setup, update, frame, copy, render, compute, and submit operations. |
| Validation | Command order, lifetimes, pass state, formats, capabilities, and malformed streams. |
| Fixtures | Positive and negative traces shared by native and WebGPU validation. |
| Packets | Binary setup/update/frame transport plus payload arenas. |
| Recording | DVZR/debug capture and replay workflows. |
| Capabilities | Runtime feature and format reporting. |

DRP2 does not own scene semantics, visual-family APIs, panel layout, controller behavior, or user
interaction policy.

## Current Surface

| Topic | Status |
| --- | --- |
| Native command streams | active |
| Fixture validation | active |
| `vklite` execution | active for scene/app runtime subset |
| WebGPU execution | experimental fixture and promoted live-route subset |
| Compute and `ResourceBarrier` | narrow experimental compute-to-render slice |
| Full output conformance | deferred |

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

- [Runtime internals](runtime-internals.md)
- [WebGPU subset](../reference/webgpu-subset.md)
- [Compute and graphics](../reference/compute-graphics.md)
- [Record and replay frame streams](../how-to/record-replay.md)
- [Adding a DRP2 command](../contributors/adding-a-drp2-command.md)
