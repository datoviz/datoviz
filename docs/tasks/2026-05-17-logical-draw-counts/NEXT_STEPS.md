# Next steps: make logical draw counts universal

## 1. Define the metadata contract

Keep these fields in `DvzFramePlanVisualMeta`:

```c
uint32_t vertex_count;
uint32_t index_count;
uint32_t instance_count;
```

Contract:

- nonzero values are authoritative logical counts for the frame.
- zero values mean fallback derivation from resource size is allowed only for raw/legacy paths.
- retained buffer capacity must never be treated as logical item count when metadata provides counts.

## 2. Populate counts per visual family

In `_scene_visual_frame_plan_metadata()` / nearby helpers, set counts for:

1. point / pixel / marker: logical item count; WGSL lowering may turn this into instances.
2. segment: current derived cache vertex/index count. Done in first slice.
3. path:
   - unstroked path: point/vertex count.
   - stroked path: derived segment cache vertex/index count.
4. primitive: vertex count, or index count when indexed.
5. mesh: vertex/index count and instance count.
6. image / glyph: current derived quad/triangle vertex count.
7. sphere: item count.
8. volume: fixed generated geometry count.
9. future vector/tube/text visuals: require logical counts from first implementation.

## 3. Prefer metadata counts in lowering

In `src/scene/visual_pipeline.c`, consistently use:

```c
if (meta->vertex_count > 0)
    out->vertex_count = meta->vertex_count;
else
    derive from buffer size;
```

Same pattern for `index_count` and `instance_count`.

## 4. Add shrink/grow regression tests

For each retained/dynamic family, add focused tests that:

1. create a visual with a larger logical count;
2. emit and capture `DRAW` / `DRAW_INDEXED` count;
3. update to a smaller logical count;
4. emit again;
5. assert draw count shrank;
6. grow again and assert draw count grows.

Priority order:

1. segment (already has first regression via axes);
2. stroked path;
3. point / pixel / marker;
4. image / glyph;
5. mesh / indexed primitive.

## 5. Document the invariant near the code

Add short comments near `DvzFramePlanVisualMeta` and typed visual lowering:

- retained resources may over-allocate or preserve capacity;
- uploads describe byte writes;
- visual metadata describes logical draw counts;
- draw commands must use logical counts when available.

## 6. Later cleanup

After all typed families are wired, remove count-from-capacity assumptions from typed scene paths.
Keep byte-size derivation only for raw FramePlan fixtures and compatibility paths.

## Suggested validation

```bash
just build
just test test_axis
just test scene
 git diff --check
```

Use narrower filters for each visual family as tests are added.
