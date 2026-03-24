# DRP2 Fixtures

This directory contains canonical DRP2 conformance fixtures for the active `2.0` contract.

Fixtures are both:

1. authoritative machine-readable test vectors,
2. worked examples that explain how the spec should be interpreted.


## Layout

The first corpus is split into:

1. `negative/`
2. `positive/`
3. `negative_schema/`

The immediate priority is `negative/`.
Those fixtures lock the failure boundary for the active DRP2 `2.0` command set, schemas, error
codes, and lifetime/state rules.


## First Corpus

The first negative corpus should stay intentionally small and cover the core validation surface:

1. duplicate id rejection,
2. unsupported major protocol version during handshake,
3. active command issued before explicit handshake completion,
4. active command issued after a failed handshake,
5. unknown id rejection,
6. wrong object type rejection,
7. draw outside render pass,
8. dispatch outside compute pass,
9. copy inside a pass,
10. finishing an encoder with an open pass,
11. pass-kind mismatch,
12. destroying a resource still referenced by recorded work,
13. buffer range violation,
14. texture range violation,
15. texture mip-level violation,
16. invalid texture transfer layout metadata such as undersized row stride, undersized rows-per-image,
    or insufficient write payload footprint,
17. missing command discriminator,
18. wrong field names,
19. missing required command fields,
20. unsupported texture format,
21. unsupported sample count,
22. draw without a bound render pipeline,
23. dispatch without a bound compute pipeline,
24. render-only state command used in a compute pass,
25. resubmitting an already submitted command buffer,
26. draw missing a required vertex-buffer binding,
27. indexed draw missing an index-buffer binding,
28. bind group set before any pipeline is bound,
29. destroying a bind group still referenced by recorded work,
30. bind-group entry with incompatible resource kind,
31. bind-group entry with incompatible resource usage bits,
32. bind-group entries that do not match the declared layout,
33. bind group bound into a pipeline slot expecting a different layout,
34. bind group dynamic offsets missing, extra, or applied in the wrong layout order,
35. dynamic buffer binding created without an explicit buffer range,
36. destroying a bind-group layout or pipeline still referenced by recorded work,
37. destroying a bind-group layout or pipeline still referenced by already submitted work,
38. pipeline rebind followed by SetBindGroup validated against the newly bound pipeline,
39. pipeline rebind followed by Draw validated against the newly bound pipeline.

Positive fixtures can follow once the negative corpus and fixture envelope are frozen.

The first positive corpus should stay minimal and focus on clean command shapes:

1. buffer upload only,
2. successful `HelloRenderer` plus `RendererHelloReply` negotiation,
3. texture upload only,
4. command encoder plus compute pass plus successful finish,
5. buffer-to-texture copy,
6. texture-to-buffer copy,
7. queue submit of a finished command buffer,
8. render pass with pipeline bind plus draw,
9. compute pass with pipeline bind plus dispatch,
10. render pass with pipeline plus vertex and index bindings for `DrawIndexed`,
11. render pass with pipeline plus bind-group binding before draw,
12. render pass with bind-group dynamic offsets applied in layout order before draw,
13. render pass pipeline rebind with refreshed bind-group and vertex-buffer state before draw.


## Metadata Policy

Fixtures may include human-readable metadata such as:

1. `description`
2. `reason`
3. `fix`
4. `notes`

These fields are non-normative.
They exist to make the corpus readable and reviewable.
Only the normative fields described in `FORMAT.md` determine conformance.


## Schema-Negative Policy

Fixtures under `negative_schema/` are expected to fail during `schema_validation`.

Those fixtures must still satisfy the fixture envelope itself.
Only the embedded DRP command objects are intentionally malformed.


## Reuse

Fixtures should remain backend-agnostic and reusable by both native and browser runtimes.

See `FORMAT.md` for the fixture envelope and naming rules.
See `schema/drp_fixture.schema.json` for the machine-readable fixture envelope.
See `RUNNER.md` for the minimal runner and result-matching contract.
