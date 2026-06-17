# Adding A DRP2 Command

DRP2 commands are backend contracts. Add one only when an existing command cannot express a
portable runtime operation.


## Before Adding A Command

Read the DRP2 authority documents:

1. `spec/drp2/README.md`;
2. `spec/drp2/AUTHORITY.md`;
3. `spec/drp2/READING_ORDER.md`;
4. the relevant schema, fixture, or command reference under `spec/drp2/`.

Prefer extending a descriptor, payload field, or validation rule over adding a command when the
runtime operation is already the same.


## Required Surfaces

A new command normally touches:

| Surface | Requirement |
| --- | --- |
| schema/spec | command shape, field semantics, defaults, unsupported cases |
| C stream API | typed builder function and validation |
| fixtures | positive and negative command-stream examples |
| scene emission | only if scene can produce the command through frame-plan lowering |
| native runtime | vklite execution or explicit unsupported diagnostic |
| WebGPU runtime | execution, fixture parity, or explicit unsupported diagnostic |
| docs | command reference and feature/status notes when public |

Do not make scene code emit backend-specific variants. Backends should consume the same command
stream or fail with a capability diagnostic.


## Validation

Use the narrowest DRP2 loop while iterating:

```sh
just build
just test drp2
just spec-check
git diff --check
```

If the command affects runtime execution, add the relevant scene, vklite, WebGPU fixture, or
readback smoke. If it affects shaders or bind layouts, also run `just shader-abi-check`.
