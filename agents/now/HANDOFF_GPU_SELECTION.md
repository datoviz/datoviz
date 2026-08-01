# GPU Selection Test Handoff

Status: implementation pending. The design direction is approved; keep implementation commits separate from the completed Release logging fix.

## Goal

Make native GPU tests select an actual Vulkan physical device through Datoviz instead of changing loader order or relying on hard-coded GPU index `0`. The immediate physical target is the Windows 11 hybrid-GPU laptop with AMD Radeon 780M Graphics and NVIDIA GeForce RTX 5060 Laptop GPU, followed by a reusable cross-platform test contract.

## Verified Baseline

On the physical Windows machine, normal Vulkan enumeration currently exposes AMD Radeon 780M Graphics as index `0` and NVIDIA GeForce RTX 5060 Laptop GPU as index `1`. `VK_LOADER_DRIVERS_SELECT=*amd*` and `VK_LOADER_DRIVERS_SELECT=*nv*` were both verified with `vk/gpu_props`, but loader filtering is only a driver-isolation diagnostic and must not become the primary Datoviz selection mechanism.

The public runtime already supports explicit index selection through `dvz_gpu_ctx_config_gpu()`. The missing boundary is the test runner: `dvztest` has no GPU option, `dvz_gpu_ctx_config()` defaults to index `0`, and the current test tree contains many default context constructors plus at least 20 directly hard-coded index-zero queue, device, canvas, or live-smoke paths.

## Recommended Initial Contract

Add these user-facing test commands:

```text
dvztest --list-gpus
dvztest --gpu <index> [existing filters and options]
```

Optionally accept `DVZ_TEST_GPU=<index>` for CI and shell convenience. Precedence must be `--gpu`, then `DVZ_TEST_GPU`, then the existing default index `0`. Invalid, negative, overflowing, or unavailable indices must fail before selected GPU tests begin and must print the available devices.

`--list-gpus` should print the enumeration index, device name, integrated/discrete type, vendor ID, device ID, API version, and driver version. A normal selected run should print the resolved device once before cases execute.

Do not make `dvz_gpu_ctx_config()` read test environment variables. Its public default constructor must remain deterministic for library users, and production behavior must remain unchanged.

## Test-Runner Boundary

Extend the internal `TstOptions` and `TstContext` path with a numeric selected GPU index and an accessor available to test code. `--gpu` is an option with a value and is not parent-only, so the existing shard and process-isolation argument forwarding should propagate it automatically to child processes.

Keep Vulkan-specific enumeration and validation in the Datoviz test adapter rather than importing Vulkan into the generic runner core. A focused helper boundary should provide at least:

```c
uint32_t dvz_testing_gpu_index(const TstContext* ctx);
DvzGpuCtxConfig dvz_testing_gpu_ctx_config(const TstContext* ctx);
```

The configuration helper should start from `dvz_gpu_ctx_config()`, apply `dvz_gpu_ctx_config_gpu()`, and return the configured value. Tests that already have a `TstContext` should use this helper instead of constructing an unselected default context.

Low-level tests that manually create `DvzInstance`, query queue capabilities, or build `DvzDeviceConfig` must use `dvz_testing_gpu_index(ctx)` consistently for both the queue query and device configuration. Canvas fixtures, vklite present fixtures, offscreen fixtures, scene GPU contexts, app test contexts, and `dvz_live_canvas` require the same audit; selecting an index in only the high-level helper would leave mixed-device paths behind.

## Discovery Mode

The unified `dvztest` executable should own `--list-gpus` first. It may create a minimal `DvzInstance`, enumerate `DvzGpuInfo`, print the table, destroy the instance, and exit without registering or running cases. Focused component runners may accept `--gpu` immediately; duplicating discovery logic into every focused runner is not required for the first implementation.

If preprocessing `--list-gpus` in `dvztest.c` would duplicate option parsing unsafely, add a narrow generic early-option callback to `TstSuite` instead of adding Vulkan dependencies to `testing.cpp`. Prefer the smaller boundary after inspecting the final diff.

## Report Evidence

Machine-readable JSON must identify the requested GPU index and resolved device metadata: name, device type, vendor ID, device ID, API version, and driver version. Prefer one top-level run metadata object rather than repeating identical data in every case. Preserve backward readability of the current schema; if existing consumers reject unknown fields, bump the schema version and update its tests deliberately.

Process-child and shard JSON aggregation must reject inconsistent GPU metadata rather than silently merging results from different devices. Human-readable failure output should include the selected GPU name.

## Deferred `--gpu all`

Do not fold `--gpu all` into the first implementation unless the index-selection patch remains small and its report semantics are explicit. A later `--gpu all` mode should enumerate once in the parent, launch a separate isolated run per GPU, retain GPU identity in each result set, and avoid accidentally combining same-named cases from different devices.

The simplest reliable physical campaign before `--gpu all` is two explicit invocations, one per index. CPU-only coverage may be run once, while GPU/Vulkan-tagged cases are run for both devices. Audit resource tags before relying on them to exclude CPU cases because some current tests create GPU contexts indirectly.

Stable selectors such as `vendor:device`, name substring, UUID, or Windows LUID may follow numeric selection. Numeric indices are acceptable for one run after `--list-gpus`, but persistent CI should eventually select a stable identity. Adding UUID or LUID to the public `DvzGpuInfo` structure would be a public ABI and binding change and is intentionally outside the first patch.

## Validation Matrix

The first implementation is complete only when all of the following pass without `VK_LOADER_DRIVERS_SELECT`:

1. `--list-gpus` reports both physical devices on the Windows laptop with AMD at the observed index `0` and NVIDIA at the observed index `1`, or reports any changed order accurately.
2. A focused GPU property test proves that `--gpu 0` and `--gpu 1` resolve different expected device names.
3. Process-isolated tests receive the same selection as their parent.
4. Invalid GPU indices fail clearly before GPU work.
5. Debug Vulkan, vklite, DRP2, canvas, scene, app, GUI, capture, and video scopes run on each GPU as applicable.
6. Release scopes run without requiring `DVZ_LOG_LEVEL`, relying on the dedicated test-adapter fix.
7. JSON reports retain the correct device metadata after process isolation and sharding.
8. `git diff --check` passes, no `data` changes are staged, and public binding regeneration is unnecessary unless a public header is deliberately changed.

## Physical Windows Campaign After Implementation

Run the current AMD baseline first, then the NVIDIA campaign, with separate output directories and identical filters. Compare the known AMD failures in GUI, canvas video/capture, scene offscreen, and Kvazaar against NVIDIA. Preserve exact driver versions, selected device metadata, Debug and Release configuration, presentation mode, test totals, crash codes, Vulkan validation messages, and live-smoke results.

Keep `VK_LOADER_DRIVERS_SELECT` as an optional cross-check: a selected-index run and a single-driver-filter run should resolve the same named GPU. A mismatch is loader or selection evidence, not a reason to restore hard-coded index zero.

## Likely Files

- `testing/testing.cpp` and `testing/testing.h`: option parsing, context propagation, child forwarding, and JSON run metadata.
- `testing/datoviz_testing.c` and `testing/datoviz_testing.h`: Datoviz-specific selected-index and configured-context helpers.
- `testing/dvztest.c`: unified GPU discovery mode if kept outside the generic runner.
- `testing/dvz_live_canvas.c`: replace direct index zero with an explicit selected index or a dedicated CLI option.
- `src/vk/tests/`, `src/vklite/tests/`, `src/canvas/tests/`, `src/drp2/tests/`, `src/scene/tests/`, `src/app/tests/`, and `src/gui/tests/`: audit default contexts, queue queries, device configurations, and fixtures.
- Runner and scheduler tests under `testing/`: option parsing, child propagation, invalid selection, and JSON metadata coverage.

## Commit Shape

Prefer reviewable checkpoints: first runner option/context propagation and focused tests, second Datoviz helper plus hard-coded-index migration, third discovery/report metadata if it does not fit cleanly in the first two. Do not mix physical result artifacts, generated binaries, `data`, or unrelated Windows portability changes into these commits.
