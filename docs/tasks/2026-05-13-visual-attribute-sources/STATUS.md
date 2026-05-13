# Visual Attribute Sources Status

## Status

Design note recorded; implementation deferred.

## Context

The active first-slice scene API stores builtin visual data as dense retained per-item arrays through
`dvz_visual_set_data()` and `dvz_visual_set_data_range()`.
Point visuals currently require per-item `position`, `color`, and `size`; point size is read from a
vertex input and is not optimized as a uniform/constant source.

The design discussion on 2026-05-13 clarified that future scene attributes need explicit source
semantics:

1. `CONSTANT` for one value shared by the visual,
2. `PER_ITEM` for one value per item,
3. `PER_SPAN` for one value per structural span,
4. `PER_GROUP` for one value per semantic group.

The API direction is explicit semantic-range setters, not count-based inference and not byte offsets.

Normative design reference:

1. [../../../spec/scene/pipeline/ATTRIBUTE_SOURCES.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/pipeline/ATTRIBUTE_SOURCES.md)

## Validation

Documentation-only update.
`git diff --check` passed after the spec edit.
