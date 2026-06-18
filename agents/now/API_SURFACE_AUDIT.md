# Datoviz v0.4-dev Public API Surface Audit

Status: completed and reconciled on 2026-06-18.

This file records the closeout of the public API surface audit. It is no longer an active work
queue.


## Completion Summary

The actionable findings from the post-dead-code-cleanup audit were handled in:

1. `c8e7532f4` - public API contract hardening across installed headers, DRP2 builder validation,
   `dvz_box_extent()` behavior, public utility exports, VMA header leakage removal, ownership
   wording, advanced/unstable labels, and focused tests.
2. `2283206fd` - final P2 scene-doc edge cases for the duplicate `dvz_panel_add_visual()` doc block
   and `DvzQueryResult.scale` lifetime.


## Resolved P0 Items

1. Duplicate GUI flags: already resolved in the inspected tree; `gui/enums.h` now contains dialog
   enums only.
2. Vulkan C++ linkage guards: already resolved in the inspected tree for `vk/instance.h` and
   `vk/queues.h`.
3. Installed non-exported declarations: exported the intended public file I/O, box, and PRNG
   helpers.
4. VMA leakage: removed the `vk_mem_alloc.h` include from `vk/instance.h`.
5. Standalone controller structs: classified standalone controller headers as advanced/unstable.
6. DRP2 viewport/scissor docs: normalized coordinate contracts now consistently say `[0, 1]`
   attachment space.
7. `dvz_box_extent()`: implemented default, expand, and contract strategy behavior and added math
   tests.


## Resolved P1/P2 Items

Contract wording now covers app/GUI flags, GUI callback call order, disabled render no-op status,
borrowed Vulkan frame handles and file descriptors, wrapped command/image ownership, vklite integer
return conventions, fixed-capacity builders, FramePlan upload lifetimes, debug-only emitter object
ids, borrowed internal inspection pointers, bulk pointer/count rules, void-setter no-op behavior,
camera ownership, standalone arcball semantics, deferred transform/shader descriptors, file/mock
allocation ownership, RGB/RGBA image docs, `dvz_pretty_size_r()`, geometry ownership, scale-bar
aliasing, packet docs, DRP2 validation sentinels, advanced/unstable module labels, supported
layer/extension lifetimes, buffer resize semantics, reserved dat usage hints, Doxygen tag cleanup,
offscreen wording, text atlas internals, input-router nullability, reserved two-way controller
links, orbit-camera Doxygen, plot role visual escape hatches, query-result scale lifetime, and the
stale duplicate panel visual doc block.


## Validation

Passed locally:

```sh
just build
just test math
just test drp2
./build/testing/dvz_public_header_probe
./build/testing/dvz_public_header_cpp_probe
git diff --check
```

Attempted but blocked by the local Python environment:

```sh
just ctypes-check
```

Failure:

```text
ModuleNotFoundError: No module named 'clang'
```

Re-run `just ctypes-check` in an environment with the Python clang bindings available before RC1
API freeze.
