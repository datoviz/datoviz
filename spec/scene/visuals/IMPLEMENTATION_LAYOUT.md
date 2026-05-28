# Scene Visual Implementation Layout

This note records the extraction pattern learned while moving the first simple v0.4 visual
families into `src/scene/visuals/<family>/`.


## Status

Landed on 2026-05-28:

1. `point/api.c` owns `dvz_point()`, point style defaults, point style validation, and shared
   point-style material synchronization helpers.
2. `marker/api.c` owns `dvz_marker()`, marker style defaults, marker-to-point style conversion, and
   marker style validation.
3. `pixel/api.c` owns `dvz_pixel()`.
4. `sphere/api.c` owns `dvz_sphere()` and `dvz_sphere_mode()`.
5. `splat/api.c` owns `dvz_splat()`.
6. `stroke/common.c` owns shared stroke cap/join validation, stroke-quad cache release, path-stroke
   cache release, and path-backed subpath copying.
7. `segment/api.c` owns `dvz_segment()` and `dvz_segment_set_caps()`.
8. `path/api.c` owns `dvz_path()`, `dvz_path_set_caps()`, `dvz_path_set_join()`, and
   `dvz_path_set_subpaths()`.
9. `vector/api.c` owns `dvz_vector_style()`, `dvz_vector()`, `dvz_arrow()`,
   `dvz_vector_set_style()`, and `dvz_vector_set_subpaths()`.

Existing family folders already owned their GPU query implementations through `query.c`. The first
refactor step makes each family folder own the public family API and the family-local style or mode
entry points as well.


## Family Folder Contract

Use this layout for active visual families when the logic is family-specific:

```text
src/scene/visuals/<family>/
  api.c       public constructor, family-specific public setters, local validation
  query.c     GPU query/pick/probe implementation for that family
```

Additional files are appropriate once a family grows enough local logic:

```text
  lower.c     family-specific lowering helpers, once extracted from shared plan files
  upload.c    family-specific derived upload/cache helpers
  bounds.c    family-specific bounds helpers, if the shared reducer becomes too large
```

Do not create empty placeholder files. A family folder should contain only code it owns today.


## What Belongs In `api.c`

Move code into `api.c` when it is tied to one public visual family:

1. public constructor such as `dvz_point()` or `dvz_sphere()`;
2. public family setters such as `dvz_point_set_style()` or `dvz_sphere_mode()`;
3. default family descriptors such as `dvz_marker_style()`;
4. validation for family-specific style, mode, cap, or shape state;
5. tiny conversion helpers that only serve that family API.

Keep the public API functions documented with their existing Doxygen comments when moving them.
These moves should be behavior-preserving and should not rename public functions by themselves.


## What Stays Shared For Now

The first pass deliberately did not move these shared dispatch files:

1. `attrs.c`, `family.c`, and `desc.c`: installed attribute tables, descriptor inference, and
   generic data-upload contracts still span many families.
2. `material.c`: material/depth-cue/alpha state is cross-family and tied to pass capabilities.
3. `shader_desc.c`, `pipeline*.c`, `bind_desc.c`, and `pass_caps.c`: shader and pipeline selection
   still encode cross-family backend policy.
4. `bounds.c`: the current reducer is shared and may be split only after family-local reducers are
   clearer.
5. `plan/visual_lowering*.c` and `runtime/render_emit.c`: lowering and runtime emission still
   coordinate multiple visual families and DRP2 resource lifetimes.

Move these later only when the extracted family file can own a coherent slice without duplicating
dispatch policy. The target is not one file per enum case; the target is stable ownership.


## Include Pattern

Family `api.c` files normally need:

```c
#include <stdint.h>

#include "_assertions.h"
#include "_scene.h"
#include "_visual_internal.h"
#include "datoviz/scene.h"
```

Add `<math.h>`, `<stdbool.h>`, or `_log.h` only when validation needs them. Constructors that call
`_scene_alloc_visual()` need `_visual_internal.h`; forgetting it produces an implicit declaration
warning/error even though `_scene.h` is already included.


## CMake Note

`src/scene/CMakeLists.txt` currently globs `visuals/*/*.c*`, so new family-local `api.c` and
`query.c` files are picked up without editing CMake. If that glob is replaced later, keep the family
subdirectory sources explicit enough that adding a new `api.c` remains mechanical.


## Commit Checklist

For each family extraction:

1. move only one coherent family slice;
2. preserve existing comments and Doxygen blocks;
3. run `just build`;
4. run `git diff --check`;
5. run `direnv exec . just test scene`;
6. stage only the family move, not unrelated worktree changes;
7. commit before moving to the next family.

Unrelated `data`, `libs/vulkan/`, generated binaries, or concurrent WebGPU/example work must remain
unstaged unless explicitly approved for that commit.


## Next Candidates

The next useful extractions are larger than the simple families:

1. `segment`: stroke-quad derived upload/cache builders currently in
   `src/scene/plan/visual_lowering_uploads.c`.
2. `path`: path-stroke derived upload/cache builders currently in
   `src/scene/plan/visual_lowering_uploads.c`.
3. `vector`: endpoint/cache glue that delegates to segment-like or path-like stroke lowering.

Keep frame-plan orchestration in plan code. Move geometry/cache construction only when the extracted
file can own a coherent stroke-family slice without copying segment/path/vector lowering logic.
