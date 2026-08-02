# Windows CUDA/Vulkan Interop Next Steps

## Resume Route

1. Read repository `AGENTS.md`, `agents/now/START.md`, this task record, `docs/advanced/cuda-external-memory.md`, `docs/tasks/2026-05-15-cuda-cupy-interop/NEXT_STEPS.md`, and the applicable graphics/build rules before changing code.
2. Start from the latest accepted RC4 development tip on a dedicated feature branch. Verify the main worktree and `data` submodule state before editing; do not stage generated binaries, CUDA runtime payloads, reports, or `data` changes.
3. Audit the current external-handle contract in `src/common/external_handle.c`, Vulkan memory export/import in `src/vk/memory.c`, semaphore export in `src/vklite/sync.c`, CUDA tests, `tools/bindings/cuda_interop_bridge.c`, and the CUDA examples before choosing helper boundaries.

## Recommended Implementation Sequence

### Phase 1: Native descriptor and ownership abstraction

1. Introduce the smallest reusable internal helper for filling `cudaExternalMemoryHandleDesc` and `cudaExternalSemaphoreHandleDesc` from `DvzExternalHandle` plus the Vulkan handle type.
2. Keep Linux FD and Windows HANDLE ownership semantics explicit in types, docstrings, cleanup paths, and tests. Avoid scattered `#ifdef` descriptor construction where one focused module can own it.
3. Reject unsupported handle kinds and device UUID mismatches before importing or launching CUDA work.

### Phase 2: Native Windows proof

1. Remove the Linux-only CMake gate from the CUDA external-buffer example only when its source has a complete Win32 path.
2. Port the Vulkan-memory round-trip/timeline test to opaque Win32 memory and timeline-semaphore handles.
3. Port the DRP2 external vertex-buffer render/readback proof without introducing a parallel renderer or CPU synchronization substitute.
4. Add repeated import/write/signal/wait/render/cleanup coverage. Use timeline values monotonically and insert the consumer-specific Vulkan visibility barrier before vertex or transfer reads.

### Phase 3: Physical Windows campaign

1. Build focused Debug and Release targets with MSVC and CUDA enabled.
2. Enumerate Vulkan devices and record CUDA/Vulkan UUID identity. Run the positive path on the NVIDIA GPU and the explicit unavailable path on AMD.
3. Run focused `vk`, `vklite`, and `drp2` tests under Vulkan validation, repeat the native example, inspect normal close/reopen, and check process HANDLE counts or another suitable leak signal across repeated runs.
4. Preserve concise text/JSON results outside Git, then summarize exact commit, toolchain, CUDA/Vulkan versions, GPU/driver, commands, totals, skips, validation messages, and human observations in this task record.

### Phase 4: Python/CuPy parity

1. Extend `tools/bindings/cuda_interop_bridge.c` to accept Win32 memory and timeline-semaphore HANDLEs with correct lifetime rules.
2. Remove the Python smoke's Linux guard only after platform capability detection and Windows cleanup are implemented.
3. Run export-only, CuPy write, semaphore synchronization, DRP2 render/readback, repeated-frame, and teardown tests in a clean Windows Python environment.
4. Keep the high-level API experimental; do not freeze `datoviz.cuda_array()` or another public convenience API as part of the platform port unless separately approved.

### Phase 5: Documentation and closeout

1. Update `docs/advanced/cuda-external-memory.md`, `docs/reference/gpu-array-interop.md`, example metadata, and platform capability wording with the exact implemented boundary.
2. State that buffers are shared directly while the initial image route uses a shared linear pixel buffer followed by a GPU-only Vulkan buffer-to-texture copy; direct shared `VkImage` support remains out of scope.
3. Run `git diff --check`, binding regeneration/checks if public headers or binding policy changed, focused native/Python validation, and repository/submodule audits before each checkpoint commit.
4. Update `STATUS.md` and this file with commits, validation evidence, remaining risks, and the next exact command before handing off.

## Stop Conditions

Stop and ask the maintainer before expanding the task to CUDA-owned allocations, direct shared images, a stable public Python API, new binary dependencies in release artifacts, an RC4 release-blocker designation, or changes that affect non-NVIDIA behavior outside explicit capability reporting.
