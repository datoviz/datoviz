# DRP2 Reading Order

Read these files in order during DRP2 review.


## 1. Orientation

1. [README.md](README.md): directory status, map, validation, and scope.
2. [AUTHORITY.md](AUTHORITY.md): conflict resolution and source-of-truth order.
3. [LAYER1.md](LAYER1.md): primary human-readable contract overview.
4. [GLOSSARY.md](GLOSSARY.md): fixed protocol terminology.
5. [VERSIONING.md](VERSIONING.md): compatibility and contract-evolution rules.


## 2. Active Protocol Surface

1. [COMMANDS.md](COMMANDS.md): active `2.0` command surface and command semantics.
2. [LIFETIMES.md](LIFETIMES.md): object lifetime and encoder/pass state rules.
3. [ERRORS.md](ERRORS.md): validation and error model.
4. [CAPABILITIES.md](CAPABILITIES.md): feature and format capability reporting.
5. [CONFORMANCE.md](CONFORMANCE.md): conformance levels and requirements for `2.0`.


## 3. Machine-Readable Review Material

1. [schema/README.md](schema/README.md): schema authority, active files, and maintenance rules.
2. [schema/drp_command.json](schema/drp_command.json): root union for active commands.
3. [schema/commands/](schema/commands/): per-command schema files.
4. [schema/common/](schema/common/): shared enums and common value types.
5. [schema/DEFERRED.md](schema/DEFERRED.md): explicitly deferred schema inventory.


## 4. Executable Conformance Material

1. [fixtures/README.md](fixtures/README.md): fixture corpus overview.
2. [fixtures/FORMAT.md](fixtures/FORMAT.md): fixture file format.
3. [fixtures/RUNNER.md](fixtures/RUNNER.md): fixture runner behavior and usage.
4. [fixtures/positive/](fixtures/positive/): canonical valid traces.
5. [fixtures/negative/](fixtures/negative/): canonical invalid semantic traces.
6. [fixtures/negative_schema/](fixtures/negative_schema/): canonical invalid schema traces.


## 5. Pressure And Roadmap Notes

1. [recording/README.md](recording/README.md): DRP2/DVZR recording and replay design status.
2. [roadmap/README.md](roadmap/README.md): long-horizon WebGPU and native/browser parity direction.
