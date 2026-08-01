# Native Test GPU Selection Handoff

Status: automated implementation and Linux multi-GPU validation complete; physical Windows AMD/NVIDIA validation remains pending. Updated: 2026-08-01.

## Implemented Contract

- Test runners accept strict numeric `--gpu N`, `DVZ_TEST_GPU=N`, and discovery-only `--list-gpus`.
- CLI selection overrides the environment; absent selection preserves GPU index `0`.
- Selection is immutable suite state propagated through fixtures, subprocess isolation, shards, reports, and `dvz_live_canvas`.
- JSON reports record requested index, resolved index, selection source, device name, vendor/device IDs, API/driver metadata, and availability.
- CPU-only listing and filtering do not initialize Vulkan; invalid selection fails before cases execute; missing Vulkan preserves CPU passes and explicit GPU skips.
- Ordinary vk, vklite, DRP2, Canvas, scene, app, GUI, capture, and applicable video paths use the selected device.
- Enumeration, deliberate default/invalid-index cases, CUDA UUID matching, and independent encoder selection follow explicit exemptions rather than claiming homogeneous selected-device proof.
- Release-mode adapter behavior no longer depends on `DVZ_LOG_LEVEL`.

Linux multi-GPU validation is green, including distinct device identity, direct and child execution, sharding, report aggregation, invalid selectors, live Canvas selection, and focused subsystem scopes.

## Deferred `--gpu all`

A future `--gpu all` should enumerate once in the parent and launch one isolated run per GPU, preserving identity in each result set. Stable UUID, LUID, vendor/device, or name selectors may follow numeric selection but are not required for v0.4.

## Remaining Physical Windows Campaign

On the AMD/NVIDIA laptop:

1. Run `--list-gpus` and record actual ordering, names, vendor/device IDs, APIs, and drivers.
2. Run identical focused Debug scopes with `--gpu 0` and `--gpu 1`, keeping separate output directories and reports.
3. Cover vk, vklite, DRP2, Canvas, scene, app, GUI, capture, applicable video, and `dvz_live_canvas` offscreen/GLFW paths.
4. Run Release scopes without relying on `DVZ_LOG_LEVEL`.
5. Keep intentional enumeration, production-default, invalid-index, CPU-only, CUDA/UUID, and encoder cases separate according to their exemption policy.
6. Compare known AMD failures with NVIDIA and preserve exact test totals, skips, crash codes, validation messages, presentation behavior, and selected-device metadata.
7. Optionally cross-check each selected-index run with `VK_LOADER_DRIVERS_SELECT`; a mismatch is evidence to investigate, not a reason to restore hard-coded device zero.

Do not mix physical result payloads, generated binaries, `data`, or unrelated portability changes into code commits.
