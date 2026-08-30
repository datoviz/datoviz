# Differential code QA campaign

Status: in progress on isolated local branch `qa/differential-febbb0142`. Started: 2026-08-31.

This record captures differential correctness and robustness evidence for production changes since the completed exploratory source audit. It is mutable-tree development evidence, not exact-artifact, hosted-platform, or release-candidate proof.

## Scope and plan

The audit compares baseline `545c9937989ab2faf075155b3d7bfd38a74b0933` with initial campaign head `febbb0142f261a6526febec8ac2a64556377bb20` in isolated worktree `/home/cyrille/GIT/Viz/datoviz-qa-differential`.

1. Inventory changed production paths and review them by correctness risk and subsystem contract.
2. Run differential compiler and static-analysis checks, inspecting underlying diagnostics directly.
3. Reproduce credible findings with focused tests, then make small validated fixes and checkpoint commits.
4. Run bounded CPU sanitizer, Valgrind, concurrency, recovery, and runtime-object-lifetime checks where local provider boundaries permit attributable evidence.
5. Run the relevant focused and broad native, DRP2, vklite, specification, binding, installed-consumer, and presentation gates.
6. Record exact results, limitations, commits, and remaining external gates; finish with whitespace and repository-hygiene checks.

Three parallel review agents are read-only. The primary QA agent owns all edits, validation decisions, evidence updates, and commits.

## Repository state

- Initial branch: `qa/differential-febbb0142`.
- Initial HEAD: `febbb0142f261a6526febec8ac2a64556377bb20` (`docs: align Point Cloud publication state`).
- Prior audit baseline: `545c9937989ab2faf075155b3d7bfd38a74b0933` (`docs: refresh combined app API reference`).
- Remote: `origin`, configured as `git@github.com:datoviz/datoviz.git` for fetch and push; the user authorized a final push to `origin/main` if needed after safe integration, but the isolated QA branch will not be published.
- `data` state at bootstrap: uninitialized gitlink at `b94d32d9c0a0a4c47e7e5c393b4ccc570159ed96`; it will remain uninitialized, unmodified, unstaged, and uncommitted.
- Git commit preflight: passed in the isolated worktree before repository edits.

## Environment

| Item | Value |
| --- | --- |
| Host | `fractal` |
| Operating system | Ubuntu 24.04.4 LTS, Linux `7.0.0-28-generic` |
| Architecture | x86_64 |
| CPU | Intel Core i9-14900K, 24 cores and 32 logical CPUs |
| Primary GPU | NVIDIA GeForce RTX 5090, 32607 MiB, proprietary driver 595.84 |
| Additional Vulkan devices | Intel RPL-S with Mesa 25.2.8; llvmpipe with Mesa 25.2.8 |
| Vulkan instance | 1.4.328 |
| NVIDIA Vulkan device API | 1.4.329, conformance 1.4.3.3 |
| Display state at bootstrap | `DISPLAY` unset; surface information unavailable from `vulkaninfo --summary` |
| GCC / default C compiler | GCC 13.3.0 |
| Clang / clang-tidy | 18.1.3 |
| CMake / Ninja | CMake 3.28.3; Ninja 1.11.1 |
| cppcheck / Valgrind | cppcheck 2.13.0; Valgrind 3.22.0 |
| just / Python / Node | just 1.58.0; Python 3.12.3; Node 22.17.1 |
| Unavailable optional analyzers | scan-build, Infer, flawfinder |

Resolved build options, compilation databases, sanitizer flags, Vulkan validation configuration, shader/provider details, and display-backed execution context will be recorded with their corresponding checks.

## Differential inventory

The exact range contains 153 commits and changes 662 files with 38,110 insertions and 34,487 deletions.

| Area | Changed files |
| --- | ---: |
| `src/**` | 181 |
| `include/**` | 13 |
| `tools/**` | 61 |
| `examples/**` | 55 |
| `testing/**` | 10 |
| `docs/**` | 186 |
| `spec/**` | 92 |
| Other top-level/build/release paths | 64 |

Changed `src` files by subsystem: scene 126; wasm 7; vklite 6; drp2 6; canvas 6; window 5; gui 5; fileio 4; common 4; app 4; input 3; geom 3; vk 2.

Documentation prose is excluded from this campaign except where a specification defines a contract needed to judge production behavior. Changed build, validation, packaging, example, and documentation tooling remains in scope when it can conceal failures or produce false-green evidence.

## Initial risk classification

| Risk | Primary changed surfaces | Review emphasis |
| --- | --- | --- |
| Ownership and stable identity | scene resources, mesh replacement, texture resizing, fonts and atlases, DRP2 named objects, window registries | Retained IDs across growable storage, borrowed versus owned objects, atomic replacement, rollback, idempotent destruction |
| Recovery and synchronization | canvas fences, swapchain errors, runtime reset, scene artifact re-realization, app presentation | Fence state after failures, command ownership, retry demand, light and texture restoration, partial-frame cleanup |
| Allocation and bounds | scene payloads, file I/O, geometry, input arrays, external handles, serialization | Checked count and byte arithmetic, narrowing conversions, offsets and strides, malformed inputs, allocation failure unwind |
| Runtime-object churn | scene realization, descriptors, pipelines, textures, render products, view lifecycle | Bounded steady-state counts across resize, replacement, reset, repeated create/destroy, and shutdown |
| Callback and thread lifetime | input routing, window callbacks, logging/thread wrappers, shader/provider first use | Removal during dispatch, instance-scoped state, synchronization ownership, provider boundaries |
| Command and image state | vk, vklite, canvas, drp2, app | Recording ownership, render-pass state, layouts, submission, acquired-frame abandonment, swapchain recreation |
| Test and runner reliability | testing helpers, Python/Node/shell tools, WebGPU and gallery staging | Child exit propagation, timeout handling, raw analyzer status, cache invalidation, false greens, deterministic failure evidence |
| Public/install surface | public headers, bindings, CMake/pkg-config/FetchContent, package metadata and licenses | Installed completeness, generated binding freshness, ABI width, prerelease versioning, runtime asset/provider discovery |

## Checkpoint results

### Checkpoint 1: differential inventory and risk review

Status: complete.

The exact inventory, environment, available analyzers, and initial risk map are recorded above. Three read-only review lanes covered scene lifetime/resource behavior, the runtime graphics foundation, and CPU/tooling/package reliability. Reproducer-based review confirmed two moderate lifetime/recovery defects, one low allocation-unwind defect, and one low installed-header contract defect. One high-impact DRP2 lifetime inconsistency remains a material architecture decision and is intentionally unchanged pending maintainer direction.

### Checkpoint 2: differential static analysis

Status: complete with a bounded cppcheck limitation.

- A normal GCC Debug build with validation and CUDA enabled completed all 1,266 steps without compiler diagnostics after explicitly initializing every required non-`data` submodule.
- A separate Clang 18 Debug build with validation enabled and CUDA disabled completed all 1,257 steps without compiler diagnostics.
- Differential clang-tidy covered 91 changed native translation units from the compilation database and completed successfully. The 57 unique diagnostics comprised enum-sentinel test values, layout padding, portability warnings, five dead stores, and one path-sensitive null warning. Manual call-graph review found no attributable correctness defect; the null path in `render_emit_prepare.c` is unreachable through all current internal callers.
- Differential cppcheck was deduplicated to 91 native translation units and bounded at 300 seconds. It reached 71 units, or 78%, before timeout. The partial result was dominated by missing-system-include, const/style, C++ vendored-header, and parser-derived null-path noise, so it is recorded as incomplete supporting evidence rather than a pass or a release gate.
- scan-build, Infer, and flawfinder were unavailable locally.

### Checkpoint 3: sanitizer and Valgrind analysis

Status: pending.

### Checkpoint 4: runtime validation and stress

Status: pending.

## Findings

### Fixed: canvas timeline and swapchain state after generic present failure

Severity: moderate. A successful queue submission followed by a generic presentation error returned failure while leaving `canvas->timeline_value` stale and the swapchain runtime ready. With a one-slot swapchain, the next frame could signal the same timeline value and reuse synchronization state without first rebuilding. A deterministic forced-status test reproduced the stale timeline. The fix records the submitted timeline value immediately after successful queue submission and marks generic present errors out-of-date. The focused regression and all 13 display-backed GLFW canvas recovery tests pass under Xvfb with Vulkan validation enabled.

### Fixed: scene buffer retirement could be queued without frame demand

Severity: moderate. Destroying a scene buffer detached direct visual and compute consumers before requesting a frame. Retirement was queued, but no new frame was necessarily emitted to carry it. A regression test proved the missing frame revision and retirement emission. The fix requests affected figures before detachment; focused buffer destruction, geometry replacement, and buffer tests pass.

### Fixed: panel creation ignored default-light reverse-edge allocation failure

Severity: low. If default-light reverse-edge growth failed, `dvz_panel()` returned a partially initialized panel without inherited default lights and consumed a panel slot. An injected allocator failure reproduced the successful return. Panel-light initialization now reports failure; creation detaches partial edges, restores the count, clears the slot, and returns `NULL`. The focused failure-unwind, light ownership/upload, and figure slot-reuse tests pass.

### Fixed: routed union-event installed-header contract omitted text events

Severity: low. The installed header documented pointer, keyboard, resize, and scale union events but omitted text events even though the implementation, tests, and normative keyboard-input specification include them. The public header and generated ctypes documentation now include text. `just ctypes`, `just ctypes-check`, and all selected input tests pass; one unrelated GUI case skipped because the initial non-Xvfb invocation could not create a window.

### Decision required: DRP2 destruction semantics after queue submission

Severity: high if the documented contract is authoritative. Current semantic validation permits buffer destruction once recorded command buffers have been submitted, while `spec/drp2/LIFETIMES.md` says submitted resources remain in use for the rest of the stream because the protocol has no completion primitive. Backends currently avoid immediate failure through backend-specific waiting or deferred destruction, but the semantic layer has no per-resource completion provenance and a live bind group may retain a logically destroyed buffer. Restoring conservative rejection is the preferred release-candidate choice; formally permitting early logical destruction requires a larger completion/provenance design and dependent bind-group validation.

## Commits

- `a8edead51` `qa: record differential campaign inventory`
- `01d1e5ca2` `fix(scene): request frames before buffer detach`
- `43239e0c3` `fix(canvas): recover safely from present errors`
- `280f369e8` `fix(scene): unwind panel light allocation failure`
- `d6b4627ec` `docs(input): include text in routed event contract`

## External and exact-artifact exclusions

This campaign does not claim physical Windows or macOS proof, hosted-platform proof, exact source-archive or wheel proof, immutable-candidate proof, publication proof, or completion of release gates owned outside this local mutable-tree audit.
