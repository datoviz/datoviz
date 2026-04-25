# Deferred DRP2 Schemas

This file lists schema files that are present in-tree but are currently deferred and non-authoritative.

Use `../COMMANDS.md` and `README.md` in this directory as the source of truth for the active protocol
surface under review.

Deferred command schema files:

- `commands/CreatePipelineLayout.json`
- `commands/DestroyPipelineLayout.json`
- `commands/CreateSampler.json`
- `commands/DestroySampler.json`
- `commands/CreateTextureView.json`
- `commands/DestroyTextureView.json`
- `commands/ResourceBarrier.json`
- `commands/DispatchWorkgroupsIndirect.json`
- `commands/DrawIndirect.json`
- `commands/DrawIndexedIndirect.json`

Deferred means:

1. the filename may reserve a future command name,
2. the JSON shape may be incomplete, stale, or incompatible with the active prose contract,
3. reviewers must not treat these files as compatibility commitments,
4. implementations must not rely on them until they are promoted into the active list.


## Promotion Criteria

A deferred command can be promoted into the active surface when:

1. field semantics are frozen in prose in `../COMMANDS.md`,
2. the corresponding JSON schema is updated to match that prose,
3. `drp_command.json` root union is updated to include the command,
4. at least one positive and one negative fixture covers the new command,
5. any new error codes or capability fields required by the command are defined in `../ERRORS.md`
   and `../CAPABILITIES.md`.

Promotion must keep schema, fixtures, and prose in lockstep per the rules in `../VERSIONING.md`.


## Promotion Risk For `2.0`

Two deferred groups are candidates for promotion before `2.0` ships, depending on pressure-test
outcomes (see the pressure tests in `../LAYER1.md`):

- `CreateTextureView` / `DestroyTextureView`: may be required if the texture-upload-and-sampling
  pressure test cannot be expressed cleanly with render-pass attachments referencing textures
  directly.
- `CreateSampler` / `DestroySampler`: may be required by the same pressure test if texture
  sampling in a shader needs an explicit sampler object.

All other deferred commands are lower priority and expected to target `2.1` or later.
