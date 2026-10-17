# Canonical Input Routing

Status: RC3 correction implemented locally on `fix/input-router-gesture-ownership`; not merged or published. Main implementation commit: `3690cc8f8`. Updated: 2026-10-17.

## Result

One router-owned interpreter replaces independent recognizers subscribed by each activation handle. Windows retain one activation; GUI viewports borrow the source router. Each original event reaches typed and union subscribers once. Newly derived gestures precede original delivery. Frozen audiences, registration-order removal, explicit nested dispatch, and failure atomicity are defined in [the normative input routing contract](../../spec/scene/interaction/INPUT_ROUTING.md).

The two initial canonical cardinality/lifecycle regressions failed before the fix. Nine input regressions now cover cardinality, overlapping click/drag ownership, semantic passthrough, long press, nested audience changes, stable removal, and allocation failures. Window, GUI and real panzoom regressions exercise production ownership and controller paths. Exported signatures and public layouts are unchanged; generated Python docstrings match the headers.

## Validation

- Linux NVIDIA, Debug build with ASan disabled: full native suite **1,224/1,224 passed, zero failures or skips**, including all 23 input tests and 15 GUI tests under Xvfb.
- Valgrind on all 23 input tests: **zero errors, zero bytes retained**, including partial snapshot allocation failure.
- `just ctypes` and `just ctypes-check`: pass, including 207 ABI records; 15 focused Python raw/facade/GUI smoke tests pass.
- Python diagnostic: zero, one and two activation handles each yield **one raw and one union wheel event** with unchanged timestamp and delta.
- Independent GPT-6.1 Sol high source audit found no remaining P1/P2 after test-driven sentinel and allocator-fixture corrections. Source review complements the recorded runtime checks.

Native `main` lacks scientific-3D APIs required by the current IBL package; an exploratory run against it failed on missing APIs including `dvz_mesh_set_fragment_clip`. Compatibility evidence instead uses an isolated exact `f77b5d05f914149858d56c374e7cb69ee23a72b3` source plus the eight-file core patch from `3690cc8f8`, applied without context edits. That build and regenerated bindings pass; focused input/window/controller/GUI runs pass 24/32/30/25 tests with zero skips. Candidate library SHA256: `2c4167aa7185ca06998b392220d46d4786c9fe08ecaa8ff2ae75a650be1430d0`.

IBL package validation on Python 3.11 passes **1,087 tests with one explicitly unmaterialized registered-asset-set skip**. An actual XTest GUI run at UI scale 1.5 verifies hover identity, all three region mappings, one wheel notch changing AP **660→658**, and a 16-notch burst ending at **626** with matching accepted contour requests. Six polygon buffers retain their identities; the bounded cache holds eight entries (7,272,224 bytes). The one-notch trace has one `step_slice()` and one preparation, with 511.63 ms settle time; this single run is functional/timing evidence, not a controlled comparative benchmark.

At the input-only checkpoint, the pre-wheel mapping transition still logged `CreateBuffer InvalidState`. The buffer-growth correction below resolves this independent resource-lifecycle issue in the local candidate. The original supported native binary was rehashed unchanged; its checkout, configuration and immutable package pin were preserved.

## Buffer-growth correction

Local follow-up commit `cb4859066` fixes the mapping failure. The scene uploads UV data for untextured meshes without binding it to submitted draws; later capacity growth previously failed because runtime replacement eligibility required historical submitted use. `CreateBuffer` now also permits replacement of buffers defined by an earlier successful command stream. Newly defined unused duplicates within one execution, wrong kinds, destroyed ids, pending captures/readbacks, and incompatible dependent bindings remain rejected. External registrations do not gain this eligibility. The [command contract](../../spec/drp2/COMMANDS.md#createbuffer) records the deliberate buffer-only amendment.

Native provenance is tracked privately and promoted only in cloned validation state; failed validation/backend execution cannot commit it. WebGPU direct, checked and packet-set paths commit touched definitions only after their full public execution succeeds, including checked GPU error scopes. Existing WebGPU partial resource-mutation behavior is unchanged; this does not introduce general rollback.

The native persistent-mesh regression and focused WebGPU regression failed before the fix. Afterward the full native suite passes **1,231/1,231 with zero skips**. All six new CPU lifecycle tests pass Valgrind with **zero errors and zero retained bytes**; the mesh executes nine growth/shrink updates on one native runtime. Specification checks pass, including WebGPU runner smoke (42 fixtures, two streams, 89 negatives) and command coverage. Independent high Sol 6.1 review found no remaining P1/P2 after external ownership and checked GPU-failure cases were covered. Browser GPU execution is not established by Node smoke.

The isolated f77 scientific candidate with both fixes passes **1,087 IBL tests with one optional registered-asset-set skip**. An actual XTest trace runs Allen/Beryl/Cosmos three times, then hover, one-notch and 16-notch burst checks, with **zero runtime execution errors**; the prior failing UV buffer grows successfully without resetting the runtime. Accepted contour requests, AP 660→658→626, and six fixed polygon buffers remain correct. This supersedes the unresolved mapping-error gate above for the local candidate. The original native binary, configuration and immutable IBL pin remain unchanged.

## Remaining gates

Physical macOS/Windows input and interactive maintainer review remain unverified. Local Linux success does not approve release artifacts or baseline promotion. Keep the change local until the maintainer reviews the exact final commits and approves merge/publication; IBL pin promotion is a separate decision.
