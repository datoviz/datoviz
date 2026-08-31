# DRP2 Fixtures

This directory contains canonical DRP2 conformance fixtures for the active `2.0` contract.

Fixtures are both:

1. authoritative machine-readable test vectors,
2. worked examples that explain how the spec should be interpreted.


## Layout

The active corpus is split into:

1. `negative/`
2. `positive/`
3. `negative_schema/`

The negative fixtures lock the failure boundary for the active DRP2 `2.0` command set, schemas,
error codes, and lifetime/state rules. The positive fixtures lock clean command shapes and portable
runtime pressure cases. The schema-negative fixtures keep command-schema validation explicit without
making malformed command payloads look like semantic failures.


## Active Corpus

The negative corpus should stay focused and cover the core validation surface:

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
12. destroying a buffer still referenced by open or unsubmitted recorded work, including one of multiple captures,
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
30. bind-group entry with structurally incompatible resource kind,
31. bind-group entry with incompatible resource usage bits,
32. bind-group entries that do not match the declared layout,
33. bind group bound into a pipeline slot expecting a different layout,
34. bind group dynamic offsets missing, extra, or applied in the wrong layout order,
35. dynamic buffer binding created without an explicit buffer range,
36. destroying a bind-group layout, shader module, or pipeline still referenced by recorded work,
37. destroying a bind-group layout, shader module, or pipeline still referenced by already submitted
    work,
38. pipeline creation with a shader module whose stage does not match the pipeline slot,
39. pipeline creation with a shader module that declares an unsupported required feature,
40. pipeline rebind followed by SetBindGroup validated against the newly bound pipeline,
41. pipeline rebind followed by Draw validated against the newly bound pipeline.
42. texture-to-buffer copies with invalid mip selection or inconsistent layout metadata.
43. duplicate handshake commands after negotiation has started,
44. draw or dispatch commands targeting passes that already ended,
45. copy commands targeting encoders that were already finished.
46. diagnostic `Error` commands before stream start are invalid,
47. diagnostic `Error` commands after handshake failure remain valid,
48. diagnostic `Error` commands in a ready session do not by themselves poison later active
    commands.
49. queue submission with the same command buffer id listed more than once,
50. queue submission with an empty command-buffer list.
51. texture view created with an unknown parent texture id,
52. texture view destroyed while still referenced by recorded work,
53. buffer replacement while a capture or readback is pending,
54. readback buffer destruction before reply validation,
55. bind-group rebinding that revalidates a destroyed non-dynamic buffer dependency,
56. duplicate pending readback submission ids, malformed or incorrectly sized reply data, and out-of-order replies.

The positive corpus should stay minimal and focus on clean command shapes:

1. buffer upload only,
2. successful `HelloRenderer` plus `RendererHelloReply` negotiation,
3. texture upload only,
4. command encoder plus compute pass plus successful finish,
5. buffer-to-texture copy,
6. texture-to-buffer copy,
7. queue submit of a finished command buffer,
8. shader-module creation only,
9. render pass with pipeline bind plus draw,
10. compute pass with pipeline bind plus dispatch,
11. render pass with pipeline plus vertex and index bindings for `DrawIndexed`,
12. render pass with pipeline plus bind-group binding before draw,
13. render pass with bind-group dynamic offsets applied in layout order before draw,
14. render pass pipeline rebind with refreshed bind-group and vertex-buffer state before draw,
15. texture view lifecycle: create texture, create view, bind via bind group, destroy in reverse order,
16. buffer destruction after ordinary submission, after every one of several captures submits, and with an unrelated open encoder,
17. buffer replacement after submitted capture,
18. readback buffer destruction after a matching reply consumes the request.


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
