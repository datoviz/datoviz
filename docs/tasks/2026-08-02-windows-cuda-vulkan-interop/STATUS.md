# Windows CUDA/Vulkan Interop For RC4

Status: planned

## Summary

RC4 should add and physically validate the experimental Windows/NVIDIA counterpart of Datoviz's existing Linux/NVIDIA CUDA/Vulkan external-memory path. This is a scheduled RC4 engineering lane, not an RC4 release blocker unless the maintainer explicitly promotes it.

The supported direction remains Vulkan-owned buffers exported to CUDA. Do not broaden this task into importing arbitrary CUDA-owned pointers into Vulkan, direct shared `VkImage` support, portable WebGPU compute, or a general CUDA framework.

## Existing Substrate

- The Linux implementation exports opaque-FD Vulkan memory and a timeline semaphore, imports them into CUDA, writes a shared buffer, signals CUDA-to-Vulkan readiness, and consumes the buffer through Vulkan/DRP2.
- The Vulkan allocator, semaphore, GPU-context extension selection, and external-handle cleanup layers already contain Win32 HANDLE support. The working Windows NVENC path exercises part of this OS/Vulkan handle substrate but does not prove CUDA compute interop.
- The Windows development machine has CUDA Toolkit 13.2, Vulkan SDK 1.4.350.0, an NVIDIA GeForce RTX 5060 Laptop GPU, and an AMD Radeon 780M. CUDA interop must select and validate the NVIDIA Vulkan device by UUID; the AMD device is an expected unavailable/mismatch case, not a second CUDA target.
- The public CUDA external-memory guide and Python/CuPy smoke remain correctly scoped to Linux/NVIDIA until the Windows implementation and physical proof are complete.

## Planned Outcome

1. Add a platform-neutral CUDA import layer that maps Datoviz external-memory and timeline-semaphore exports to opaque FD descriptors on Linux and opaque Win32 HANDLE descriptors on Windows.
2. Preserve explicit handle ownership: CUDA consumes transferred Linux FDs, while Datoviz or the importing bridge closes Windows handles according to the Win32 import contract without leaks, double closes, or use-after-close behavior.
3. Build and run the native CUDA external-buffer example on Windows/MSVC without weakening the Linux path.
4. Port the low-level external-memory/timeline test and the DRP2 external vertex-buffer render/readback proof to Windows.
5. Match CUDA and Vulkan physical devices by UUID and report an actionable unavailable result when the selected Vulkan device is not CUDA-compatible.
6. Add repeated-frame, teardown, resize or re-creation where applicable, and validation-layer coverage on the NVIDIA path; retain the AMD mismatch/unsupported result as an explicit capability case.
7. Extend the Python CuPy bridge and smoke to Win32 handles only after the native C contract is stable. Keep the feature advanced and experimental.
8. Update public documentation and support wording only after the implementation, automated tests, and physical Windows evidence pass.

## Acceptance Boundary

The native Windows slice is complete only when Debug and Release builds pass focused `vk`, `vklite`, and `drp2` tests on the NVIDIA Vulkan device with no unexpected failure, crash dialog, Vulkan validation message, leaked external HANDLE, or UUID ambiguity. A visible or captured CUDA-updated point-buffer example must render multiple changing frames and close normally. The AMD-selected run must reject or skip the CUDA path with an explicit device-mismatch or unsupported-capability reason.

The Python slice is separately complete only when a clean Windows environment imports the generated binding and CUDA bridge, wraps the mapped allocation in CuPy, performs CUDA writes synchronized by the external timeline semaphore, renders through Datoviz, verifies readback, repeats the operation, and tears down cleanly.

## Validation Ownership

- Linux development machine: preserve and extend the platform-neutral abstraction, Linux FD regression tests, bindings, documentation, and non-physical review.
- Physical Windows machine: MSVC configuration/build, Win32 HANDLE ownership tests, NVIDIA UUID match, AMD negative case, Debug/Release Vulkan validation, repeated execution, visible behavior, and leak/crash observation.
- Do not report Linux CI, NVENC success, compilation alone, or process exit status as Windows CUDA/Vulkan physical proof.

## Current State

No Windows CUDA/Vulkan feature support is claimed yet. The design is planned for RC4 and resumable from `NEXT_STEPS.md`.
