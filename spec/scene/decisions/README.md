# Scene Decision Records

This directory is reserved for historical scene specification decision records.

A file belongs here only when it is primarily an ADR-style record:

1. the context that forced a choice,
2. the decision that was made,
3. the consequences of that choice,
4. its date and status.

Active design work, API sketches, and implementation-facing spec addenda belong in
[../proposals](../proposals) until their settled rules are promoted into the closest specialized
spec file.


## Authority Model

Decision records have historical authority only:

1. specialized normative spec files win over decision records,
2. active proposals win over older decision records when they explicitly revise a choice,
3. decision records may explain why a rule exists, but they should not be the only place where an
   implementation-facing rule lives,
4. fully absorbed decision records should stay here only when their rationale remains useful.


## Current Status

Active scene ADR files:

1. [CONTROLLER_BINDING_MODEL.md](CONTROLLER_BINDING_MODEL.md): scene-owned opaque controller
   handles, panel dimension binding, linked panels through shared handles, and WASM-facing
   controller API constraints.

The former active design records now live in
[../proposals](../proposals). Treat that directory as the staging area for unsettled or
not-yet-promoted normative material.
