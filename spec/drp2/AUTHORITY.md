# DRP2 Authority

This file defines how to resolve DRP2 spec conflicts.


## Source-Of-Truth Order

1. Protocol prose for the active command contract:
   [COMMANDS.md](COMMANDS.md), [LIFETIMES.md](LIFETIMES.md), [ERRORS.md](ERRORS.md),
   [CAPABILITIES.md](CAPABILITIES.md), [VERSIONING.md](VERSIONING.md), and
   [CONFORMANCE.md](CONFORMANCE.md).
2. Active JSON schemas under [schema/](schema/) as the machine-checkable mirror of the prose
   contract.
3. Fixture files under [fixtures/](fixtures/) as executable examples and regression vectors.
4. Runner behavior in [fixtures/RUNNER.md](fixtures/RUNNER.md) and the repository tools that
   implement it.
5. Recording, roadmap, and feasibility notes under [recording/](recording/) and
   [roadmap/](roadmap/).

If an active schema disagrees with command, lifetime, or error prose, the prose wins until the schema
is updated. If a fixture disagrees with both prose and schema, fix the fixture or promote a contract
change through prose first.


## Ownership

[COMMANDS.md](COMMANDS.md) owns the human active/deferred command boundary. The schema README and
schema files are the checkable mirror and should not independently promote command families.

[LIFETIMES.md](LIFETIMES.md) owns object lifetime, encoder/pass state, and submitted-work rules.
[ERRORS.md](ERRORS.md) owns error selection when more than one validation failure could apply.

Fixtures should extend the active contract; they should not introduce normative behavior that cannot
be read from prose and active schemas.


## Maintenance Rules

1. Prefer executable fixture coverage over prose-only rule additions.
2. Keep DRP2 narrow; promote deferred commands only when current scene, runtime, browser replay, or
   conformance work exposes a concrete validated need.
3. Do not allow public DRP2 definitions to mention Vulkan or other backend-native handles.
4. Keep scene API design, runtime implementation sequencing, and release status out of this
   directory unless they directly constrain the DRP2 contract.
