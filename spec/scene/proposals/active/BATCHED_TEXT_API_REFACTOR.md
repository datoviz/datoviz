# Batched Text API Refactor

> **Execution Status**
> - **Status:** `ACTIVE PROPOSAL`
> - **Updated on:** `2026-06-28`
> - **Purpose:** define the pre-RC refactor direction for public semantic text before v0.4 API
>   freeze.


## Problem

The current public `DvzText` API is centered on one string per retained object:

1. `dvz_text(panel, flags)` creates one semantic text object;
2. `dvz_text_set_string(text, string)` sets one UTF-8 string;
3. `dvz_text_set_style()` and `dvz_text_set_placement()` apply to that one string.

That model is too narrow for v0.4 release use. Datoviz already needs many positioned strings for
axes, colorbars, legends, readouts, GSP/Matplotlib backend use, and v0.3-style label clouds. The
internal text visual path already accepts a string array plus per-item position, anchor, size,
color, and angle attributes. The public API should expose that semantic model instead of forcing
callers to allocate many retained text objects.


## Decision

`DvzText` should become a retained semantic text collection. A single label is the degenerate case
where `item_count == 1`.

Rules:

1. many strings at many positions belong in one `DvzText` object;
2. one string containing `\n` is one multiline text item at one anchor;
3. multiline layout must expose interline spacing instead of relying only on renderer metrics;
4. rich paragraph/card text that rasterizes to one texture remains a separate text-block or overlay
   path;
5. low-level `DvzVisual*` text remains internal or advanced/unstable, not the recommended public
   label API.

This intentionally breaks source compatibility with the first v0.4-dev text slice if needed.
Pre-RC correctness and API shape take priority over preserving the current single-string facade.


## Proposed Public Types

```c
typedef struct DvzTextItem
{
    uint32_t struct_size;
    uint32_t flags;
    const char* string;
    double position[3];
    float offset[2];
    float anchor[2];
    float size_px;
    DvzColor color;
    float angle;
} DvzTextItem;
```

`DvzTextItem` is the per-string payload. `string` is copied by retained text APIs before they
return. `position` uses the collection placement mode. `offset` is a logical-pixel nudge applied
after the anchor position resolves. `anchor` is the text-box anchor where `{0, 0}` is top-left,
`{0.5, 0.5}` is center, and `{1, 1}` is bottom-right.

```c
typedef struct DvzTextLayout
{
    uint32_t struct_size;
    uint32_t flags;
    float line_height;
    float line_gap_px;
    float wrap_width_px;
    DvzTextAlign align;
} DvzTextLayout;
```

`DvzTextLayout` is collection-wide in the first refactor. Add per-item layout only after a concrete
use case needs it.

Line spacing semantics:

1. `line_height == 0` or `1` uses the renderer/font natural line height;
2. values greater than `1` multiply the natural line height;
3. `line_gap_px` adds fixed logical pixels between adjacent lines;
4. the baseline advance is `natural_line_height * resolved_line_height + line_gap_px`;
5. one-line text ignores line-spacing fields visually.

`DvzTextAlign` should start with left, center, and right paragraph alignment. Baseline-aware anchor
values may be added later through the broader text-placement model.


## Proposed Public Functions

Required core API:

```c
DvzText* dvz_text(DvzPanel* panel, uint32_t flags);
int dvz_text_set_items(DvzText* text, const DvzTextItem* items, uint32_t item_count);
int dvz_text_set_layout(DvzText* text, const DvzTextLayout* layout);
int dvz_text_set_style(DvzText* text, const DvzTextStyle* style);
int dvz_text_set_placement(DvzText* text, const DvzTextPlacement* placement);
int dvz_text_set_renderer(DvzText* text, DvzTextRenderer renderer);
void dvz_text_destroy(DvzText* text);
```

Compatibility/convenience helpers may remain, but they must be documented as one-item sugar:

```c
int dvz_text_set_string(DvzText* text, const char* string);
int dvz_text_set_position(DvzText* text, const double position[3]);
```

Array helpers may be useful for language bindings and GSP:

```c
int dvz_text_set_strings(DvzText* text, const char* const* strings, uint32_t item_count);
int dvz_text_set_positions(DvzText* text, const double (*positions)[3], uint32_t item_count);
int dvz_text_set_offsets(DvzText* text, const float (*offsets)[2], uint32_t item_count);
int dvz_text_set_anchors(DvzText* text, const float (*anchors)[2], uint32_t item_count);
int dvz_text_set_sizes(DvzText* text, const float* sizes_px, uint32_t item_count);
int dvz_text_set_colors(DvzText* text, const DvzColor* colors, uint32_t item_count);
int dvz_text_set_angles(DvzText* text, const float* angles, uint32_t item_count);
```

These setters must validate item-count consistency. Either all per-item arrays have the same
`item_count`, or the mutation fails without partially changing retained state. The existing
`dvz_visual_set_data_many()` behavior is the internal model to preserve.


## Retained State Model

`DvzText` should retain:

1. owning scene and panel;
2. item count;
3. copied strings;
4. per-item position, offset, anchor, size, color, and angle arrays;
5. default style, renderer, placement, and layout;
6. derived glyph visual or internal text visual handle;
7. dirty versions for strings, layout/style, placement, and renderer resources.

Style resolution order:

1. per-item values override collection style when present;
2. collection style fills item defaults;
3. scene font defaults fill unresolved size and renderer policy.

Placement resolution order:

1. collection `DvzTextPlacement` defines screen/data/world placement mode, reference anchor, depth
   policy, and default angle if per-item angle is unset;
2. per-item `position` is interpreted in that placement mode;
3. per-item `offset` is a logical-pixel post-transform offset;
4. per-item `anchor` aligns the measured text box.


## Implementation Plan

1. Add `DvzTextItem`, `DvzTextLayout`, and `DvzTextAlign` to public scene headers.
2. Replace `DvzText::string`-centered retained state with copied string and per-item arrays.
3. Implement `dvz_text_set_items()` as the atomic core mutation API.
4. Reimplement single-item helpers in terms of `dvz_text_set_items()`.
5. Route public `DvzText` preparation through the existing batched text-visual realization path
   instead of building one glyph visual per text object.
6. Preserve `\n` handling for multiline items and add `DvzTextLayout` line-spacing control to both
   bitmap and atlas-backed layout paths.
7. Keep axes, legends, colorbars, scale bars, and readouts on the same batched semantic path.
8. Treat generic `DvzVisual*` text construction as internal or advanced/unstable in docs.
9. Update Python binding generation and NumPy-adaptation policy so string arrays,
   positions, colors, and sizes map cleanly.


## Example Refactor

`examples/c/visuals/text.c` should demonstrate one `DvzText` collection with several text items:

1. one retained `DvzText*`;
2. five strings;
3. five positions;
4. varied sizes, colors, anchors, and one rotated item;
5. one `dvz_text_set_items()` call.

`examples/c/features/text_block.c` should stop creating one text object per line. Pick one of two
clear examples:

1. multiline semantic label: one `DvzTextItem` whose string contains `\n`, plus explicit
   `DvzTextLayout` line spacing; or
2. rich text block: use the CPU-rasterized text-block/overlay path and rename the example if it is
   demonstrating formatted paragraph/card text rather than semantic glyph labels.

The release gallery should include both cases only if both API paths are intended to be supported
in v0.4:

1. `visual.text`: batched labels;
2. `feature.text_block`: multiline or rich block, with the distinction visible in the source.


## Tests And Validation

Required focused tests:

1. one `DvzText` with many items lowers to one batched text realization;
2. updating only strings does not recreate unrelated scene objects;
3. updating only positions takes the position-only update path where supported;
4. mismatched per-item array counts fail atomically;
5. one item with `\n` produces multiple lines at one anchor;
6. `line_height` and `line_gap_px` change measured multiline height;
7. atlas coverage is ensured for all strings in the collection;
8. item colors, sizes, anchors, and angles survive render-plan preparation;
9. old examples are migrated or intentionally removed before RC docs are generated.

Validation loop for the refactor:

```sh
just build
just test text
just test interaction
just test axis
python3 tools/check_example_manifests.py
git diff --check
```


## Non-Goals

Do not fold these into the batched-text refactor:

1. HarfBuzz/BiDi/full complex shaping;
2. TeX or math layout;
3. collision avoidance;
4. text editing, selection, or substring picking;
5. public atlas UV or glyph-run manipulation;
6. per-item rich style runs beyond size, color, anchor, and angle.


## Open Questions

1. Should `DvzTextItem::position` be `double[3]` publicly while the internal text visual uses
   `float[3]`, or should the public item also use `float[3]` for visual-family consistency?
2. Should per-item optionality use bit flags, sentinel values, or separate array setters only?
3. Should single-item helpers remain exported for ergonomics, or should v0.4 force all callers
   through `dvz_text_set_items()` and language bindings?
4. Should baseline anchoring land in this refactor or wait for the broader split-anchor placement
   cleanup described in the semantic text spec?
