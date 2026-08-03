# Native Test GPU Selection Handoff

Status: automated implementation and Linux plus source-checkout Windows AMD/NVIDIA validation complete; visible Windows asymmetry and exact-candidate physical proof remain. Updated: 2026-08-03.

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

## Remaining Physical Windows Work

The symmetric source-checkout matrices are complete. On the AMD/NVIDIA laptop, repeat only the affected visible and exact-candidate scopes described in [HANDOFF_WINDOWS_VALIDATION.md](HANDOFF_WINDOWS_VALIDATION.md):

1. Preserve separate output directories, reports, and selected-device metadata for GPU 0 and GPU 1.
2. Repeat affected Release GLFW live-canvas rendering after relevant runtime changes, resolving the earlier AMD blank frame and NVIDIA single-point result.
3. Run the exact installed-candidate scopes on both usable GPUs once the source bundle and wheel are frozen.
4. Keep intentional enumeration, production-default, invalid-index, CPU-only, CUDA/UUID, and encoder cases separate according to their exemption policy.
5. Optionally cross-check each selected-index run with `VK_LOADER_DRIVERS_SELECT`; a mismatch is evidence to investigate, not a reason to restore hard-coded device zero.

Do not mix physical result payloads, generated binaries, `data`, or unrelated portability changes into code commits.
