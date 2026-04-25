# DRP2 Conformance

This document defines what it means for a runtime to claim DRP2 `2.0` conformance.

It distinguishes three levels of conformance and states which levels are required for `2.0`.


## Conformance Levels

### Level 1 — Validation Conformance

A runtime passes Level 1 if it classifies every fixture in the active corpus correctly.

Requirements:

1. all `negative_schema/` fixtures must be rejected during schema validation with the expected error
   code,
2. all `negative/` fixtures must be rejected during semantic or capability validation with the
   expected error code at the expected command index,
3. all `positive/` fixtures must pass schema and semantic validation without error.

Verification: run the fixture corpus with `just drp2-fixtures` or
`python3 tools/drp2_fixture_runner.py`. All fixtures must report `PASS`.

Level 1 is **required** for DRP2 `2.0` conformance.


### Level 2 — Execution Conformance

A runtime passes Level 2 if it can execute every positive fixture on a GPU without producing a
protocol-level error.

Requirements:

1. the runtime must successfully execute every `positive/` fixture end-to-end,
2. for fixtures that contain a `QueueSubmit` with readbacks, the runtime must reply with a
   `QueueSubmitReply` whose shape mirrors the request exactly: same `submission_id`, same `buffer_id`,
   `offset`, and `size` entries in the same order,
3. `QueueSubmitReply.data` values are not verified against golden checksums in `2.0` because the
   current positive corpus uses stub shaders that do not produce defined pixel output,
4. WGSL is the required shader language for `2.0` execution conformance; every positive fixture uses
   WGSL,
5. a native runtime may additionally accept SPIR-V behind an explicit capability flag, but WGSL
   support is still required.

Level 2 is **required** for DRP2 `2.0` conformance but is not mechanically verified by the current
fixture runner. It is a prose commitment until a GPU execution harness is defined.


### Level 3 — Output Conformance

A runtime passes Level 3 if its readback output matches golden checksums across runtimes.

Requirements (deferred — not part of DRP2 `2.0`):

1. positive fixtures must encode expected readback checksums in their `expected` block,
2. fixture shaders must compute deterministic values rather than stubs,
3. tolerance rules must be defined for floating-point and format conversions,
4. the runner must verify `QueueSubmitReply.data` against the declared hash.

Level 3 is **deferred to DRP2 `2.1`**. Cross-backend pixel parity requires real shader payloads
and golden generation against actual hardware.


## Shader Language Requirements

For `2.0`:

1. WGSL is the mandatory shader language for all conformance fixtures,
2. SPIR-V support is optional and must be declared via `supported_shader_formats` in the
   capability report,
3. a runtime that claims `2.0` conformance must accept WGSL in `CreateShaderModule` commands.

These rules keep the conformance corpus usable by both native Vulkan runtimes and browser WebGPU
runtimes without requiring backend-specific shader compilation in the fixture set itself.


## Native vs. Browser Parity

Both a native runtime and a browser runtime claim the same `2.0` conformance if they satisfy
Level 1 and Level 2.

Differences in backend behavior that do not affect the protocol result — such as tile-based vs.
immediate-mode rendering, or GPU-vendor-specific texture layouts — are out of scope for `2.0`
conformance.

Cross-backend output parity (same readback bytes from the same fixture) is a Level 3 concern and
is deferred to `2.1`.


## Summary Table

| Level | Name        | Required for `2.0` | Mechanically verified |
|-------|-------------|--------------------|-----------------------|
| 1     | Validation  | yes                | yes — fixture runner  |
| 2     | Execution   | yes                | no — prose commitment |
| 3     | Output      | no                 | deferred to `2.1`     |
