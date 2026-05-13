# Visual Attribute Sources Next Steps

## Goal

Implement explicit attribute source semantics for builtin scene visuals without exposing GPU storage
details in the public API.

Start from:

1. [../../../spec/scene/pipeline/ATTRIBUTE_SOURCES.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/pipeline/ATTRIBUTE_SOURCES.md)
2. [STATUS.md](/home/cyrille/GIT/Viz/datoviz/docs/tasks/2026-05-13-visual-attribute-sources/STATUS.md)

## Proposed API Slice

Add public semantic-range setters:

```c
int dvz_visual_set_item_data(
    DvzVisual* visual, const char* attr_name, const void* data,
    uint32_t first_item, uint32_t item_count);

int dvz_visual_set_span_data(
    DvzVisual* visual, const char* attr_name, const void* data,
    uint32_t first_span, uint32_t span_count);

int dvz_visual_set_group_data(
    DvzVisual* visual, const char* attr_name, const void* data,
    uint32_t first_group, uint32_t group_count);

int dvz_visual_set_value(
    DvzVisual* visual, const char* attr_name, const void* value);
```

Add separate group-topology setters:

```c
int dvz_visual_set_group_ids(
    DvzVisual* visual, const uint32_t* group_ids,
    uint32_t first_item, uint32_t item_count);

int dvz_visual_set_span_group_ids(
    DvzVisual* visual, const uint32_t* group_ids,
    uint32_t first_span, uint32_t span_count);
```

Keep `dvz_visual_set_data()` and `dvz_visual_set_data_range()` as compatibility/convenience wrappers
for `PER_ITEM` if the active API surface still needs them.

## First Implementation Slice

Prefer a narrow point-visual slice first:

1. support `dvz_visual_set_value(point, "size", &size)` as `CONSTANT`,
2. keep `position` and `color` as `PER_ITEM`,
3. lower constant size to a small visual parameter/uniform resource or equivalent scene resource,
4. add a point shader/pipeline path that reads size from that constant source,
5. keep dense per-item size as the existing fallback path.

This validates the source model while avoiding the full `PER_GROUP` implementation initially.

## Code Areas

Likely files:

1. [../../../include/datoviz/scene.h](/home/cyrille/GIT/Viz/datoviz/include/datoviz/scene.h)
2. [../../../src/scene/_scene.h](/home/cyrille/GIT/Viz/datoviz/src/scene/_scene.h)
3. [../../../src/scene/scene.c](/home/cyrille/GIT/Viz/datoviz/src/scene/scene.c)
4. [../../../src/scene/converter.c](/home/cyrille/GIT/Viz/datoviz/src/scene/converter.c)
5. [../../../src/scene/glsl/point.vert](/home/cyrille/GIT/Viz/datoviz/src/scene/glsl/point.vert)
6. [../../../src/scene/tests/test_scene.c](/home/cyrille/GIT/Viz/datoviz/src/scene/tests/test_scene.c)

## Validation

For a documentation-only continuation, run `git diff --check`.
For implementation, run:

1. `just build`
2. `just test scene`
3. a focused GPU point-render smoke when the shader/pipeline path changes.
