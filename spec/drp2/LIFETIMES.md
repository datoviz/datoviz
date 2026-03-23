# DRP2 Lifetimes And State

This document defines object lifetime and encoder/pass state rules for DRP2 `2.0`.


## Object Identity

1. Persistent logical objects are identified by producer-chosen ids.
2. Ids are typed by the command that creates them.
3. Reusing an id for a different object type in the same stream is invalid.
4. Reusing an id after destruction in the same stream is also invalid in `2.0`.


## Persistent Objects

The following object kinds are persistent and require explicit destruction:

1. buffer
2. texture
3. texture view
4. sampler
5. bind group layout
6. bind group
7. pipeline layout
8. shader module
9. render pipeline
10. compute pipeline


## Scoped Objects

The following objects are scoped by begin/end commands and are not destroyed explicitly:

1. command encoder
2. render pass encoder
3. compute pass encoder


## Encoder State Machine

Command encoder states:

1. `open`
2. `finished`

Rules:

1. `BeginCommandEncoder` creates an open encoder scope.
2. recording commands are valid only within an open encoder.
3. `FinishCommandEncoder` closes the encoder.
4. no commands may target a finished encoder.
5. an encoder with an unclosed pass cannot be finished.


## Pass State Machine

Render pass states:

1. `open`
2. `ended`

Compute pass states:

1. `open`
2. `ended`

Rules:

1. only one pass may be open in an encoder at a time,
2. draw commands are valid only in an open render pass,
3. dispatch commands are valid only in an open compute pass,
4. copy commands are not valid inside a pass unless the contract later says otherwise,
5. a pass must be ended before any sibling pass begins.


## Destruction Safety

Destroy commands are validated against current usage.

Rules:

1. an object referenced by an open encoder or pass cannot be destroyed,
2. destroying an already-destroyed object is invalid,
3. destroying a parent object while dependent children still exist is invalid,
4. runtime teardown at end-of-stream is allowed, but it does not replace explicit destruction in
   `2.0`.


## Dependency Rules

At minimum, these parent-child relationships must be respected:

1. texture view depends on texture,
2. bind group depends on bind group layout and referenced resources,
3. pipeline depends on shader modules and pipeline layout.


## Validation Consequences

Lifetime violations should fail during semantic validation whenever possible.
They should not be deferred to backend execution unless the dependency can only be observed there.
