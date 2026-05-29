# Scene Visual Implementation Layout

This note records the extraction pattern learned while moving the first simple v0.4 visual
families into `src/scene/visuals/<family>/`.

For the active long-term visual-boundary architecture, including the visual-family registry,
generic runtime boundaries, and guardrails against root-level visual switches, read
[`../implementation/SCENE_VISUAL_BOUNDARY_GUARDRAILS.md`](../implementation/SCENE_VISUAL_BOUNDARY_GUARDRAILS.md).


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
10. `stroke/quad.c` owns segment stroke-quad cache construction and straight-vector endpoint/cache
    construction.
11. `stroke/path.c` owns path-stroke adjacency, flag, distance, and index cache construction for
    both path visuals and curved vector visuals.
12. `image/generated_quad.c` owns generated image/labels quad cache construction and the predicate
    for image-like visuals that need generated quads before upload.
13. `stroke/query.c` owns shared segment/path/vector query buffer allocation, offscreen target
    extent calculation, query render-state targeting, query upload metadata marking, and temporary
    stroke query geometry construction for straight and path-backed stroke lowerings.
14. `stroke/bounds.c` owns stroke-family bounds reducers for segment endpoint attributes and
    straight-vector endpoint expansion.
15. `sphere/bounds.c`, `image/bounds.c`, `glyph/bounds.c`, and `mesh/bounds.c` own the clean
    family-local visual-space bounds reducers for those families.
16. `image/query_quad.c` owns generated extent/anchor/tex-rect query geometry shared by image and
    labels query paths.
17. `volume/upload.c` owns volume transfer texture and sparse label lookup buffer construction;
    FramePlan upload node insertion remains in scene-emission support helpers.
18. `stroke/internal.h`, `image/internal.h`, `volume/internal.h`, and `bounds_internal.h` keep
    subsystem helper declarations out of the broad `_visual_internal.h` surface.

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
  lower.c     family-specific lowering helpers, once extracted from shared scene-emission files
  upload.c    family-specific derived upload/cache helpers
  bounds.c    family-specific bounds helpers, if the shared reducer becomes too large
```

Subsystem folders are appropriate when several families share a real implementation boundary:

```text
src/scene/visuals/stroke/
  common.c    cap/join validation, shared subpath copying, shared cache release
  quad.c      straight stroke-quad cache construction for segment and straight-vector lowering
  path.c      path-stroke adjacency/cache construction for path and curved-vector lowering
  query.c     shared stroke query mechanics
  bounds.c    shared stroke bounds reducers
  internal.h  private declarations for the stroke subsystem
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
4. `bounds.c`: cross-family bounds dispatch, panel projection, and generated bounds overlay
   synchronization stay in the shared reducer. Clean family-local visual-space reducers have moved
   to their family folders or to `stroke/bounds.c`.
5. `scene_emit/visual_lowering*.c`, `scene_emit/uploads.c`, `scene_emit/upload_support.c`, and
   `runtime/render_emit.c`: lowering, upload support, and runtime emission still coordinate
   multiple visual families and DRP2 resource lifetimes. Extracted family helpers should build
   data/cache payloads only; scene-emission support should keep resource-key, upload-node, and
   ordering policy.

Move these later only when the extracted family file can own a coherent slice without duplicating
dispatch policy. The target is not one file per enum case; the target is a visual-family operation
registry where generic scene/runtime/query code calls family contracts instead of adding concrete
visual branches.


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

Use narrower private headers once a helper belongs to a subsystem rather than the broad visual core:

```c
#include "stroke/internal.h"
#include "image/internal.h"
#include "volume/internal.h"
#include "bounds_internal.h"
```

Avoid adding new subsystem-specific declarations to `_visual_internal.h` unless the helper is
genuinely shared by most visual internals.


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

The stroke-family cache, query-helper, query-geometry, and bounds extractions are complete for the
current segment/path/vector slice. Keep frame-plan orchestration in `scene_emit/`; only move more
geometry/cache construction when an extracted file can own a coherent family slice without copying
dispatch policy. For broader visual-boundary work, use
[`../implementation/SCENE_VISUAL_BOUNDARY_GUARDRAILS.md`](../implementation/SCENE_VISUAL_BOUNDARY_GUARDRAILS.md).

Useful next candidates:

1. continue reducing scene-emission upload support only where the moved helper builds data or
   cache payloads and does not own FramePlan ordering;
2. consider similar private-header cleanup for image, volume, or bounds helpers if subsystem
   surfaces grow;
3. revisit duplicated path-stroke math between render-cache and query-cache builders only if a
   shared primitive can improve both paths without obscuring their output layouts.
