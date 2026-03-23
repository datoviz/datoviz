# Datoviz v0.4-dev Refactor Status - 2026-03-23

This note is a re-entry summary for the current `v0.4-dev` branch after the February-March 2026
refactor burst.


## Snapshot

- Branch: `v0.4-dev`
- Current HEAD at review time: `14f153844235` (`2026-03-23`, "DRP2 spec, fixtures, tools")
- Build status on this machine today: `just build` passed
- Test status on this machine today: `just test` passed, `146/146` tests passed
- Worktree note: no tracked edits were present, but several untracked local paths exist
  (`.codex/`, `plans/`, `scripts/`, local cert/key files, generated media, etc.), so treat the tree
  as locally dirty even though the branch tip is clean


## Executive Summary

The refactor is no longer in the "architecture only" phase. The branch now has a coherent v0.4-dev
build around active modules (`common`, `ds`, `fileio`, `math`, `thread`, `input`, `window`,
`canvas`, `stream`, `video`, `vk`, `vklite`) and that build is currently green.

The most important work completed before the pause was:

1. The codebase was split into clearer build layers and multiple focused test binaries, while still
   preserving the unified `dvztest` entry point.
2. The canvas/presentation/offscreen/video path was reworked and backed by many regression tests.
3. The public Vulkan ownership model was cleaned up substantially, especially around GPU discovery,
   `gpu_ctx`, surface/swapchain, and `proto`.
4. `vklite` now appears much closer to consuming `vk` through public contracts rather than private
   vk internals.
5. A large follow-up pass hardened `vklite` wrapper ownership and opacity across commands, rendering,
   shaders, buffer/image helpers, slots, compute, graphics, descriptors, and tests.

The branch feels like a successful mid-refactor checkpoint: the architecture is meaningfully cleaner
than v0.3-era code, but the remaining work is now mostly boundary hardening and API simplification,
not basic bring-up.


## What Changed Recently

The commit history since early February clusters into a few clear phases.

### 1. Presentation/canvas stabilization

Main period: `2026-02-10` to `2026-02-13`

Key outcomes:

- `canvas` present/acquire/recovery behavior was tightened.
- `swapchain_sink` responsibilities were pushed toward clearer `vklite` wrappers.
- Offscreen mode was refactored and tested more aggressively.
- Window wrapping / external surface support was added and exercised.
- CPU-only video sink behavior was implemented for macOS and video/canvas coupling was improved.

This phase appears to have produced a much more testable and backend-aware canvas layer.

### 2. Build/test topology cleanup

Main period: `2026-02-13`

Key outcomes:

- The codebase was split into clearer CMake target groups.
- `testing/CMakeLists.txt` now builds:
  - `dvztest_core`
  - `dvztest_vk`
  - `dvztest_canvas`
  - `dvztest_integration`
  - `dvztest`
- The unified runner still exists, but the repo now also supports narrower test entry points.

This is a useful structural milestone because it makes future refactor slices easier to validate in
isolation.

### 3. Public ownership/API cleanup in `vk`

Main period: `2026-02-13` to `2026-02-18`

Key outcomes:

- The GPU ownership/API boundary documented in [OWNERSHIP.md](/home/cyrille/GIT/Viz/datoviz/agents/done/OWNERSHIP.md)
  is marked complete.
- Public headers no longer expose `DvzGpu*`-style public traversal as before.
- Active call sites now use GPU index/info/query flows instead of public mutable GPU internals.
- Bootstrap/device/queue code was migrated toward the new ownership contract.

This looks like the largest conceptual cleanup already landed.

### 4. `vk` / `vklite` boundary refactor

Main period: `2026-02-18` to `2026-02-20`

Key outcomes:

- [VK_REFACTOR.md](/home/cyrille/GIT/Viz/datoviz/agents/now/VK_REFACTOR.md) records the current contract.
- `proto` was removed from the active tree and replaced in tests by explicit fixture helpers.
- The old bootstrap helper has now been removed and replaced by `gpu_ctx`.
- `surface` and `swapchain` were refactored around more opaque public handles/accessors.
- A large amount of `vklite` wrapper/test code was updated accordingly.

This is still the active frontier, but it is materially further along than this older snapshot
initially implied.

### 5. Wrapper hardening follow-up

Main period: `2026-03-23`

Key outcomes:

- `bootstrap` was fully replaced by `gpu_ctx`, and direct bootstrap users were removed.
- `proto` was fully removed from active test infrastructure in favor of `fixture_gpu` /
  `fixture_offscreen`.
- `surface` / `swapchain`, `commands` / `sync`, `buffers` / `images`, `rendering`, `shader`,
  `buffer_views`, `slots`, `compute`, and `graphics` all received accessor/lifecycle hardening.
- Several public `vklite` wrapper bodies are now opaque and use `*_create_wrapper()` / `*_free()`
  ownership more consistently.
- The suite grew from the earlier `141/141` green state to `146/146` passing tests on this machine.

### 6. Post-refactor stabilization

Main period: `2026-02-19` to `2026-03-02`

Key outcomes:

- Repeated screenshot test issue was fixed.
- Additional `vklite` fixes landed in commands/images/sync/proto/canvas interactions.
- Test/build wiring was adjusted for compute shader compilation.
- The last visible commit (`e8d13005`) fixes compute shader compilation options in the testing
  shader path, which suggests the branch ended the burst by cleaning up platform/tooling rough
  edges rather than by starting a new architectural direction.


## Current State By Area

### Build system

Status: healthy

- Root CMake now exposes explicit build toggles such as `DVZ_BUILD_CORE`, `DVZ_BUILD_VK`,
  `DVZ_BUILD_CANVAS`, `DVZ_BUILD_DRP2`, `DVZ_BUILD_WEBGPU`, `DVZ_BUILD_SCENE`.
- Active v0.4-dev modules are assembled from object libraries into shared outputs.
- Compile definitions/options are centralized and propagated more coherently than before.
- The build on this machine currently succeeds from a clean `just build`.

### Testing

Status: much stronger than before

- The repo still has the unified `dvztest` runner.
- In practice the test topology is now richer than some older docs imply, because dedicated
  `dvztest_*` binaries also exist.
- Today’s local run of `just test` passed all `146/146` tests.
- The suite now covers a lot of recovery/rebuild/present/offscreen behavior that would have been
  fragile earlier in the refactor.

### Canvas / window / stream / video

Status: actively refactored and now fairly well stabilized

- This stack received a lot of February attention and now has broad regression coverage.
- External surface wrapping, offscreen mode, present recovery, and video sink integration all look
  meaningfully exercised.
- This area no longer looks like the main blocker, unless new architectural goals require another
  round of cleanup.

### `vk`

Status: materially improved, ownership cleanup largely landed

- Public GPU pointer exposure was removed from the active API surface.
- Bootstrap/device/queue ownership semantics are clearer than before.
- The remaining work in `vk` seems more about supporting a stricter `vklite` boundary than about
  missing core functionality.

### `vklite`

Status: functional, with the wrapper-opacity pass mostly complete

- `vklite` is heavily exercised by tests and currently green.
- `proto` is gone from the active tree, and a broad wrapper-hardening pass has already landed.
- The last `sync` owner wrappers (`DvzFence`, `DvzSemaphore`, `DvzSubmit`) have now been made
  opaque as well.
- `DvzBarriers` remains public by design and is now documented as a mutable command-recording
  builder/config helper rather than an ownership leak.
- The remaining architectural debt is no longer mainly in `vklite`; it is now more about a final
  audit/closure pass plus the still-exposed allocator/memory surface in `vk`.

### Higher-level/scaffold modules

Status: intentionally dormant

- Many directories still exist under `include/datoviz/` and `src/`, but the current architecture and
  build focus remain on the active low-level/core/graphics stack.
- There is no sign that bringing renderer/scene/client layers online should be the immediate next
  move.


## Mismatches Or Cautions Worth Remembering

1. Some documentation still describes a simpler "single runner only" or slightly older architecture,
   while the code has moved forward. When in doubt, trust current `CMakeLists.txt`, `testing`, and
   the `agents/*.md` plans over older prose.
2. The repo contains many scaffold directories and public headers that are not truly active in the
   current v0.4-dev runtime.
3. `vklite` is green and much tighter than before; the remaining challenge is deciding where to
   stop cleanly so the public boundary is intentional rather than endlessly transitional.
4. The clearest still-sensitive public low-level exposure is now `include/datoviz/vk/memory.h`,
   while `include/datoviz/vk/queues.h` looks more like a deliberate low-level queue model.
5. The local worktree has many untracked files, so keep future changes narrowly scoped and avoid
   cleanup commands unless you intentionally want to sort local artifacts first.


## Suggested Next Steps

Recommended order:

1. Finish the remaining `vk`/`vklite` ownership-boundary work from
   [VK_REFACTOR.md](/home/cyrille/GIT/Viz/datoviz/agents/now/VK_REFACTOR.md), starting with the items
   already listed there:
   - keep `DvzBarriers` intentionally public as a builder/config type
   - verify no remaining `vklite` public structs are accidental ownership leaks
   - keep the focused idempotent-destroy / repeated-submit tests around the hardened wrappers
2. Continue the low-level cleanup in `vk`, especially around allocator/memory exposure in
   [`memory.h`](/home/cyrille/GIT/Viz/datoviz/include/datoviz/vk/memory.h), and only later decide
   whether the queue model in [`queues.h`](/home/cyrille/GIT/Viz/datoviz/include/datoviz/vk/queues.h)
   should stay public as-is.
3. Reconcile architecture/testing docs with the code as it exists now:
   - note the presence of split test binaries in addition to `dvztest`
   - note that the active `vklite` wrapper-opacity pass is effectively complete
   - keep the active-module list and runtime path documentation current
   - remove or rewrite stale statements that no longer match the refactor
4. After the boundary cleanup is complete, run a second pass on naming/lifecycle consistency across
   `canvas`, `stream`, `video`, `vk`, and `vklite` so the active API feels intentionally designed
   rather than incrementally migrated.
5. Only after that, decide whether to reactivate any higher-level layer. Right now the codebase is
   still getting high leverage from low-level cleanup, and moving upward too early would likely lock
   in interfaces that you still want to simplify.


## Practical Re-entry Plan

If resuming work now, the shortest sensible sequence is:

1. Read [OWNERSHIP.md](/home/cyrille/GIT/Viz/datoviz/agents/done/OWNERSHIP.md)
2. Read [VK_REFACTOR.md](/home/cyrille/GIT/Viz/datoviz/agents/now/VK_REFACTOR.md)
3. Inspect `testing/CMakeLists.txt`, `src/CMakeLists.txt`, and current `include/datoviz/vklite/*.h`
4. Run:
   - `just build`
   - `just test`
   - then narrower loops such as `just test vklite` or direct `dvztest_vk` work as needed
5. Pick one boundary-cleanup slice and finish it end-to-end before touching any dormant higher layer


## Bottom Line

You are returning to a branch that is in better shape than the commit titles alone suggest. The
major architectural direction is already established, the build is green, and the full test suite is
currently green. The highest-value next move is not another broad rewrite, but a disciplined finish
of the remaining `vk`/`vklite` public-boundary and lifecycle cleanup.
