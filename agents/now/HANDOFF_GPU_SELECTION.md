# GPU Selection Test Handoff

Status: automated implementation complete on 2026-08-01. Linux multi-GPU validation is green; the physical Windows AMD/NVIDIA campaign remains pending on access to that machine.

## Goal

Make native GPU tests select an actual Vulkan physical device through Datoviz instead of changing loader order or relying on hard-coded GPU index `0`. The immediate physical target is the Windows 11 hybrid-GPU laptop with AMD Radeon 780M Graphics and NVIDIA GeForce RTX 5060 Laptop GPU, followed by a reusable cross-platform test contract.

## Implemented Outcome

- The generic runner has one opaque run-adapter seam for option parsing, canonical pre-shard preparation, immutable suite/context state, root-only reporting, schema-v3 JSON metadata, and exact child-metadata comparison before process or shard aggregation.
- The Datoviz adapter implements strict `--gpu <index>`, `DVZ_TEST_GPU`, exclusive `--list-gpus`, CLI precedence, resolved `DvzGpuInfo` evidence, fail-closed selected-case classification, and context/suite GPU configuration helpers without importing Vulkan into the generic runner.
- Ordinary vk, vklite, canvas, DRP2, scene, app, GUI, Kvazaar, capture, presentation, fixture, queue-query, and manual-device paths use the selected GPU; production-default, invalid-index, external-surface, CUDA/UUID, external-memory, and CUDA/NVENC-owned identity paths are explicitly exempt where selection cannot be claimed honestly.
- `dvz_live_canvas` shares a narrow selector independent of the generic runner and uses the resolved GPU for both offscreen and GLFW device setup.
- CPU-only focused runners retain a Vulkan-free link boundary and emit `"run": {"gpu": null}` consistently.
- Local validation resolved NVIDIA RTX 5090 as index `0`, Intel RPL-S integrated graphics as index `1`, and llvmpipe as index `2`; index `1` passed full vk, vklite, app, GUI, and selected canvas/scene/video/live paths without loader filtering.
- The physical Windows Debug/Release AMD/NVIDIA campaign below remains required evidence and must not be inferred from the Linux results.

## Verified Baseline

On the physical Windows machine, normal Vulkan enumeration currently exposes AMD Radeon 780M Graphics as index `0` and NVIDIA GeForce RTX 5060 Laptop GPU as index `1`. `VK_LOADER_DRIVERS_SELECT=*amd*` and `VK_LOADER_DRIVERS_SELECT=*nv*` were both verified with `vk/gpu_props`, but loader filtering is only a driver-isolation diagnostic and must not become the primary Datoviz selection mechanism.

The public runtime already supports explicit index selection through `dvz_gpu_ctx_config_gpu()`. The missing boundary is the test runner: `dvztest` has no GPU option, `dvz_gpu_ctx_config()` defaults to index `0`, and a broad source audit currently finds well over 100 candidate default-constructor or index-zero occurrences. Many are ordinary paths that must migrate, while enumeration, invalid-index, production-default, and cross-API identity tests require deliberate exemptions rather than mechanical replacement.

## Recommended Initial Contract

Add these user-facing test commands:

```text
dvztest --list-gpus
dvztest --gpu <index> [existing filters and options]
```

Accept `DVZ_TEST_GPU=<index>` for CI and shell convenience. Precedence must be `--gpu`, then `DVZ_TEST_GPU`, then the existing default index `0`. The parser must accept ASCII decimal digits only and reject missing values, empty environment values, signs, whitespace, trailing characters, values above `UINT32_MAX`, and unavailable indices. A syntactically invalid explicit selector must fail immediately; an unavailable index must fail after enumeration but before selected GPU tests begin and must print the available devices.

`--list-gpus` should print the enumeration index, device name, complete Vulkan device type including CPU, virtual, other, or unknown values, vendor ID, device ID, API version, and raw driver version. Print API versions in human-readable form as well as their raw values. Vulkan driver-version encoding is vendor-specific, so name the portable numeric field `driver_version_raw` and treat any decoded driver string as best-effort evidence rather than a portable identity.

A normal GPU-selected run should print the resolved device once in the root process before cases execute. Shard and process children must remain silent while preserving the same metadata in their child JSON.

Do not make `dvz_gpu_ctx_config()` read test environment variables. Its public default constructor must remain deterministic for library users, and production behavior must remain unchanged.

## Activation And Existing Behavior

Do not initialize Vulkan merely because the runner supports GPU selection. Resolve and validate a device only when `--gpu` or `DVZ_TEST_GPU` is explicit, or when the filtered case set contains `TST_RES_GPU` or `TST_RES_VULKAN`. CPU-only runs, `--list`, and `--list-groups` must continue to work without a Vulkan runtime and must not print a selected device. Run-producing CPU-only JSON must contain `"run": {"gpu": null}` consistently; listing modes do not need to emit run JSON.

The no-option GPU path must remain index `0`. Preserve current skip behavior when Vulkan is unavailable and the user did not explicitly request a GPU; explicit selection without an available Vulkan device is an error. This distinction prevents the feature from turning an existing GPU skip or CPU-only pass into a global runner failure.

Case selection or an equivalent resource scan must occur before parent sharding decides whether GPU resolution is required. Resolve once in the root before spawning, resolve independently in every child, and compare the child result against the root metadata.

## Test-Runner Boundary

Extend the internal run state with the requested GPU index, whether it was explicit, its selection source, and resolved metadata. Make the selected index available from both `TstContext` and `TstSuite`: ordinary tests receive a context, but registered fixture constructors receive only a suite. Copy or reference the same immutable per-process run state from every case context rather than maintaining separate values.

`--gpu` is an option with a value and is not parent-only, so the existing shard and process-isolation argument forwarding should retain the selector and value automatically. Add focused forwarding tests rather than relying on that implementation detail. Environment-only selection is inherited by children and must receive the same propagation and metadata checks.

Keep Vulkan-specific enumeration, validation, naming, and `DvzGpuInfo` handling in the Datoviz test adapter rather than importing Vulkan into the generic runner core. Add a narrow Datoviz run-adapter or pre-run hook that receives parsed selection state, resolves metadata after case filtering and before execution or spawning, stores the result on the suite, and supplies it to JSON reporting and child-metadata comparison. Do not duplicate ad hoc preprocessing in every focused executable.

A focused helper boundary should provide at least:

```c
uint32_t dvz_testing_gpu_index(const TstContext* ctx);
DvzGpuCtxConfig dvz_testing_gpu_ctx_config(const TstContext* ctx);
uint32_t dvz_testing_suite_gpu_index(const TstSuite* suite);
DvzGpuCtxConfig dvz_testing_suite_gpu_ctx_config(const TstSuite* suite);
```

The configuration helpers should start from `dvz_gpu_ctx_config()`, apply `dvz_gpu_ctx_config_gpu()`, and return the configured value. Tests that already have a `TstContext` should use the context helper. Process-, worker-, or case-scoped fixture constructors should use the suite helper. Test-local fixture helpers such as `dvz_fixture_gpu()` should accept selection state rather than silently constructing GPU `0`.

Low-level tests that manually create `DvzInstance`, query queue capabilities, or build `DvzDeviceConfig` must use `dvz_testing_gpu_index(ctx)` consistently for both the queue query and device configuration. Canvas fixtures, vklite present fixtures, offscreen fixtures, scene GPU contexts, app test contexts, and `dvz_live_canvas` require the same audit; selecting an index in only the high-level helper would leave mixed-device paths behind.

The generic runner currently owns parsing, filtering, JSON writing, process isolation, and shard aggregation. The implementation must establish the adapter seam once rather than letting `testing.cpp`, `dvztest.c`, and focused runners develop separate sources of truth. Plain numeric selection and resolved metadata may cross the seam; Vulkan handles and Vulkan-specific structs must remain in `datoviz_testing.c`.

## Selection Policies And Exemptions

Do not mechanically replace every index zero or default constructor. Classify every candidate path during the audit:

1. Ordinary selected-device tests must use the runner selection for all context, queue, device, canvas, scene, app, capture, and presentation setup.
2. Enumeration and property tests may intentionally inspect one device or all devices; when the case is meant to describe the selected device, index through the test helper rather than `gpus[0]`.
3. Default-constructor and invalid-index tests must preserve their deliberate values and must not be reported as proof that an explicitly selected nonzero GPU executed their production-default path.
4. CUDA, external-memory, or other cross-API interop tests that identify a Vulkan device by UUID must retain identity matching. They may run under an explicit selector only when the matched Vulkan identity agrees with it; otherwise skip with an explicit mismatch reason or classify the case outside the selected-device campaign.
5. Video encoders and direct-Vulkan test helpers that maintain an independent device choice must either adopt the same selected physical device or be excluded explicitly from the claim that `--gpu` controls them. A Vulkan index alone does not select a CUDA or NVENC device.

Use an explicit test tag, policy field, or similarly reviewable mechanism for intentional selection exemptions. A full `--gpu 1` report must not silently mix ordinary work on GPU `1` with exempt work on production-default GPU `0`. Either exclude such cases from the per-device campaign, split their device-independent semantics from GPU execution, or represent their actual identity separately.

The initial source audit must inventory default `DvzGpuCtxConfig` construction, `gpus[0]`, queue-capability index zero, `DvzDeviceConfig` index zero, `dvz_interop_gpu_ctx(0, ...)`, direct Vulkan enumeration, and test-local GPU factories. After migration, any remaining hard-coded selection in GPU test paths must be documented inline as intentional and covered by the exemption policy.

## Discovery Mode

The unified `dvztest` executable should own `--list-gpus` first. It may create a minimal non-validating `DvzInstance`, enumerate `DvzGpuInfo`, print the table, destroy the instance, and exit before constructing or registering the suite. Focused component runners must accept `--gpu` immediately through the shared runner boundary; duplicating discovery output into every focused runner is not required for the first implementation.

Treat `--list-gpus` as an exclusive discovery action and define its behavior with other run-producing options deliberately. A missing Vulkan runtime, failed instance creation, or zero enumerated devices must produce a clear diagnostic and nonzero exit rather than an empty successful campaign.

If recognizing `--list-gpus` in `dvztest.c` would duplicate option parsing unsafely, use the shared Datoviz run-adapter or a narrow generic early-option callback instead of adding Vulkan dependencies to `testing.cpp`. The selected index, discovery output, run metadata, and focused-runner behavior must still come from one parser and one precedence rule.

## Report Evidence

Machine-readable JSON must use one top-level run metadata object rather than repeating identical data in every case. Prefer an explicit schema-version bump with the existing `summary` and `cases` keys unchanged and an optional object shaped conceptually as follows:

```json
{
  "run": {
    "gpu": {
      "requested_index": 1,
      "selection_source": "cli",
      "resolved_index": 1,
      "name": "NVIDIA GeForce RTX 5060 Laptop GPU",
      "device_type": "discrete",
      "vendor_id": 4318,
      "device_id": 0,
      "api_version_raw": 0,
      "driver_version_raw": 0
    }
  }
}
```

The example numeric values are structural placeholders, not expected hardware evidence. Preserve backward readability for consumers that access the existing top-level summary and case arrays, update the runner schema tests deliberately, and record `gpu: null` consistently for runs that do not resolve a device.

Process-child and shard JSON aggregation must read and compare GPU metadata before deleting child reports or merging case results. Equality must cover resolved index, device type, vendor ID, device ID, API version, driver version, and name. Reject missing or inconsistent metadata when the parent resolved a GPU rather than silently merging results from different devices. Human-readable failure output and the run header should include the selected GPU name.

## Deferred `--gpu all`

Do not fold `--gpu all` into the first implementation. A later `--gpu all` mode should enumerate once in the parent, launch a separate isolated run per GPU, retain GPU identity in each result set, and avoid accidentally combining same-named cases from different devices.

The simplest reliable physical campaign before `--gpu all` is two explicit invocations, one per index. CPU-only coverage may be run once, while GPU/Vulkan-tagged cases are run for both devices. Audit resource tags before relying on them to exclude CPU cases because some current tests create GPU contexts indirectly.

Stable selectors such as `vendor:device`, name substring, UUID, or Windows LUID may follow numeric selection. Numeric indices are acceptable for one run after `--list-gpus`, but persistent CI should eventually select a stable identity. Adding UUID or LUID to the public `DvzGpuInfo` structure would be a public ABI and binding change and is intentionally outside the first patch.

## Validation Matrix

The first implementation is complete only when all of the following pass without `VK_LOADER_DRIVERS_SELECT`:

1. `--list-gpus` reports both physical devices on the Windows laptop with AMD at the observed index `0` and NVIDIA at the observed index `1`, or reports any changed order accurately.
2. Strict parser tests cover CLI and environment precedence, missing and empty values, negative and signed inputs, whitespace, trailing characters, `UINT32_MAX` overflow, and unavailable indices.
3. A focused GPU property test proves that `--gpu 0` and `--gpu 1` resolve different expected device names, a helper-created `DvzGpuCtx` proves its actual selected identity, and one manual queue plus `DvzDeviceConfig` path proves it uses the same physical device.
4. Process-isolated cases and shard children receive the same selection as their parent, remain silent, and preserve matching metadata.
5. Invalid selectors fail clearly before cases execute and print discovery evidence when enumeration succeeded.
6. No-option GPU runs retain index `0`; CPU-only filters, `--list`, and `--list-groups` do not initialize Vulkan; a machine without Vulkan retains existing CPU pass and implicit GPU skip behavior.
7. Intentional enumeration, invalid-index, production-default, CUDA/UUID-matched, and independent encoder cases follow their documented policy and cannot create a falsely homogeneous per-device report.
8. Debug Vulkan, vklite, DRP2, canvas, scene, app, GUI, capture, and applicable video scopes run on each GPU. `dvz_live_canvas --gpu 0` and `dvz_live_canvas --gpu 1` both report and use the requested device in offscreen and GLFW modes where available.
9. Release scopes run without requiring `DVZ_LOG_LEVEL`, relying on the dedicated test-adapter fix.
10. JSON reports retain the correct device metadata after direct, process-isolated, and sharded execution and reject deliberately inconsistent child metadata in a runner test.
11. Help output documents `--gpu`, `--list-gpus`, and `DVZ_TEST_GPU`; focused runners accept the selector through the same shared contract.
12. The narrow runner, scheduler, and GPU tests pass, followed by `just build`, the relevant `just test` scopes, and `git diff --check`; no `data` changes are staged, and public binding regeneration is unnecessary unless a public header is deliberately changed.

## Physical Windows Campaign After Implementation

Run the current AMD baseline first, then the NVIDIA campaign, with separate output directories and identical selected-device filters. Run explicitly exempt CPU-only, production-default, enumeration, or cross-API identity tests separately according to their policy rather than mixing them into the per-device comparison. Compare the known AMD failures in GUI, canvas video/capture, scene offscreen, and Kvazaar against NVIDIA. Preserve exact driver versions, selected device metadata, selection source, Debug and Release configuration, presentation mode, test totals, skips and reasons, crash codes, Vulkan validation messages, and live-smoke results.

Keep `VK_LOADER_DRIVERS_SELECT` as an optional cross-check: a selected-index run and a single-driver-filter run should resolve the same named GPU. A mismatch is loader or selection evidence, not a reason to restore hard-coded index zero.

## Likely Files

- `testing/testing.cpp` and `testing/testing.h`: strict numeric option parsing, immutable suite and context run state, child forwarding, and generic JSON integration points.
- `testing/datoviz_testing.c` and `testing/datoviz_testing.h`: Datoviz-specific resolution, discovery, metadata, selected-index, suite/context configuration helpers, and the runner adapter.
- `testing/dvztest.c`: unified GPU discovery mode if kept outside the generic runner.
- `testing/dvz_live_canvas.c`: add the shared `--gpu` and environment contract, use the selected index for queue and device construction, and print resolved identity.
- `src/vk/tests/`, `src/vklite/tests/`, `src/canvas/tests/`, `src/drp2/tests/`, `src/scene/tests/`, `src/app/tests/`, and `src/gui/tests/`: audit default contexts, queue queries, device configurations, and fixtures.
- `src/video/tests/` and interop tests: classify direct-Vulkan, encoder, CUDA, UUID-matched, and intentionally independent device selection.
- Runner and scheduler tests under `testing/`: option parsing, CPU-only activation, child propagation, inconsistent-child rejection, invalid selection, and JSON metadata coverage.

## Commit Shape

Prefer reviewable checkpoints that each leave coherent behavior:

1. Land strict shared parsing, suite/context/fixture propagation, Datoviz discovery and resolution, root/child behavior, report metadata, aggregation checks, help, and focused runner tests together. Do not expose an accepted `--gpu` option that does not yet select or report a device.
2. Land the Datoviz helpers and ordinary selected-device migration in focused subsystem commits, correcting resource flags and asserting actual context/device identity as each scope moves.
3. Land `dvz_live_canvas` selection plus the explicit enumeration, default, invalid-index, interop, and video exception audit, then run the cross-platform automated matrix.
4. Run and record the physical Windows AMD/NVIDIA campaign separately after implementation commits pass; do not mix physical result artifacts into code commits.

Do not mix generated binaries, `data`, unrelated Windows portability changes, or unapproved physical result payloads into these commits.
