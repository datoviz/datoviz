# Proto Fixture Migration Plan

Status: `COMPLETED`

Owner slice: `vklite` test infrastructure cleanup

Date: `2026-03-23`

Verified on: `2026-03-23`

Validation on this revision:

1. `just build`
2. `direnv exec . just test vklite`
3. `direnv exec . just test`

Completion snapshot:

1. `fixture_gpu` and `fixture_offscreen` are the active `vklite` test helpers
2. `DvzProto` symbols and direct `proto.h` test usage are gone from the active tree
3. `src/vklite/proto.h` and `src/vklite/proto.c` were deleted
4. migrated tests now use explicit fixture getters for device/allocator/commands/rendering/images


## Goal

Replace the internal `proto` test helper with explicitly named fixture helpers that separate:

1. GPU bring-up ownership and device/allocator access
2. Offscreen render-target / command / screenshot infrastructure

The replacement should improve naming, make test intent clearer, and finish the remaining `proto`
boundary cleanup tracked in [VK_REFACTOR.md](/home/cyrille/GIT/Viz/datoviz/agents/done/VK_REFACTOR.md).


## Why this change

Current `proto` is useful, but it mixes several responsibilities in one object:

1. GPU bring-up / device / allocator ownership
2. offscreen color/depth attachment setup
3. rendering attachment state
4. barriers and command buffers
5. pipeline helper state
6. screenshot staging/export

That is acceptable as a temporary helper, but it is a poor long-term abstraction because:

1. the name `proto` is vague
2. tests depend on its struct layout
3. it hides two different ownership domains behind one object
4. it encourages wide field access instead of narrow fixture APIs


## Naming decision

Use the `fixture_` prefix, not `proto`, and not suffix-style `*Fixture`.

Target names:

1. `DvzFixtureGpu`
2. `DvzFixtureOffscreen`

Target function family:

1. `dvz_fixture_gpu(...)`
2. `dvz_fixture_gpu_*()`
3. `dvz_fixture_offscreen(...)`
4. `dvz_fixture_offscreen_*()`

Reasoning:

1. `fixture` makes the test/support role explicit immediately
2. prefix form makes these helpers harder to confuse with runtime API
3. names remain understandable without becoming overly long


## Target ownership split

### `DvzFixtureGpu`

Responsibilities:

1. create/destroy GPU-context bring-up state
2. create/manage device
3. create/manage allocator
4. expose queue/device/allocator access needed by tests or dependent fixtures

Non-responsibilities:

1. no render target images
2. no rendering attachments
3. no command buffer recording helpers
4. no screenshot staging


### `DvzFixtureOffscreen`

Responsibilities:

1. create/destroy offscreen color/depth images and views
2. configure `DvzRendering`
3. manage barriers and command buffers for offscreen tests
4. provide optional graphics/slots convenience for test setup
5. provide screenshot export helper for the owned offscreen color target

Non-responsibilities:

1. does not own offscreen rendering policy
2. borrows `DvzFixtureGpu`


## File layout

Fixtures should live under `src/vklite/tests/` because they are test-support infrastructure, not
runtime `vklite` API.

Planned files:

1. `src/vklite/tests/fixture_gpu.h`
2. `src/vklite/tests/fixture_gpu.c`
3. `src/vklite/tests/fixture_offscreen.h`
4. `src/vklite/tests/fixture_offscreen.c`

`proto.{h,c}` stays temporarily during migration, then is deleted entirely.


## API direction

Keep fixture APIs narrow and explicit.

Expected surface:

1. constructor / destroy pair for each fixture
2. getters for borrowed runtime objects (`bootstrap`, `device`, `alloc`, `queue`)
3. getters for owned offscreen infrastructure (`rendering`, `barriers`, `cmds`, `color`,
   `color_view`, `depth`, `depth_view`)
4. a small number of convenience helpers where they clearly reduce boilerplate:
   - graphics setup helper
   - slots getter/helper
   - image transition helper
   - PNG screenshot helper

Avoid:

1. exposing fixture struct layouts publicly
2. recreating a giant catch-all helper under a new name
3. adding fixture helpers for behavior that is specific to only one test


## Migration order

1. Add `fixture_gpu`
2. Add `fixture_offscreen`
3. Migrate [test_graphics.c](/home/cyrille/GIT/Viz/datoviz/src/vklite/tests/test_graphics.c) first
4. Adjust fixture APIs based on the first migration if needed
5. Migrate [test_techniques.c](/home/cyrille/GIT/Viz/datoviz/src/vklite/tests/test_techniques.c)
   incrementally
6. Remove all remaining `#include "proto.h"` uses
7. Delete `src/vklite/proto.h`
8. Delete `src/vklite/proto.c`
9. Run full validation and update the active refactor notes


## Completion criteria

This migration is complete only when all of the following hold on the same revision:

1. there is no `DvzProto` symbol left in the repository
2. there is no `#include "proto.h"` left in the repository
3. `src/vklite/proto.h` is deleted
4. `src/vklite/proto.c` is deleted
5. `just build` passes
6. `just test vklite` passes
7. `just test` passes


## Constraints

1. fixture helpers are internal test infrastructure only
2. no installed public headers are added for this migration
3. no dormant higher-level modules are touched
4. prefer short but readable names; use standard words like `device`, `alloc`, `cmds`, `color`,
   `depth`, `rendering`, `barriers`, `png`
5. keep the refactor disciplined: migrate consumers first, then delete `proto`


## Notes for the next implementation pass

1. Start with a minimal fixture surface, not a speculative large API
2. Let the `test_graphics.c` migration validate the naming and ownership split
3. Keep `test_techniques.c` migration incremental to avoid a large destabilizing rewrite
4. Delete `proto` only after all users are moved
