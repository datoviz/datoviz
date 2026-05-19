# Scene Text, Glyph, and Math Rendering Plan

> **Execution Status**
> - **Status:** `PLANNING NOTE`
> - **Updated on:** `2026-05-16`
> - **Purpose:** define a staged implementation path for retained glyph and text visuals,
>   high-quality Unicode text, GPU-accelerated font rendering, and later minimal math/equation
>   support in the active scene -> DRP2 -> vklite/app path.


## Context

Datoviz v0.4 already has the active rendering path that text should use:

1. `scene` owns retained objects, frame plans, panel mapping, interaction state, and DRP2 emission.
2. `drp2` owns backend-agnostic command streams, validation, recording, and native runtime execution.
3. `vklite` and `canvas` own Vulkan resources, borrowed frame targets, command buffers, and frame
   submission.
4. `app` owns the presentation loop over scene, canvas, and the DRP2 runtime.

Do not add a parallel renderer, presentation layer, Vulkan wrapper, or scene bypass for text.
Text, glyphs, labels, annotations, axes, colorbars, and equations should all lower into the same
scene -> frame-plan -> DRP2 -> vklite path as point, primitive, mesh, path, and image visuals.

Current scene text objects are bookkeeping-only:

- `include/datoviz/scene/text.h`
- `include/datoviz/scene/annotation.h`
- `include/datoviz/scene/types.h`
- `src/scene/text_annotation.c`

These retained objects are useful starting points, but they do not yet shape, lay out, cache, emit,
or render glyphs.


## Detailed Follow-Up Plans

Keep this file as the umbrella roadmap. Two focused notes now own the concrete implementation
details for the most important next slices:

- [SCENE_TEXT_ATLAS_CACHE_PLAN.md](SCENE_TEXT_ATLAS_CACHE_PLAN.md): atlas generation, cache keys,
  embedded default atlases, dynamic growth, missing glyph behavior, and optional persistent cache.
- [SCENE_HARFBUZZ_SHAPING_PLAN.md](SCENE_HARFBUZZ_SHAPING_PLAN.md): UTF-8 shaping, glyph ids,
  clusters, font fallback, layout integration, and future complex-script support.

The high-level architecture remains: HarfBuzz produces shaped glyph runs, the atlas/cache layer
ensures renderer resources for those glyphs, and the scene emits normal DRP2 resources and draws.


## External Technology Snapshot

This plan assumes the external text-rendering landscape as of `2026-05-16`.

- FreeType remains the practical font loading, glyph metrics, hinting, and bitmap rasterization
  library.
- HarfBuzz is the practical shaping engine for Unicode text, OpenType features, complex scripts,
  fallback shaping paths, and bidirectional-script integration.
- HarfBuzz now documents an experimental `libharfbuzz-gpu` component that encodes glyph outlines
  for GPU rasterization using the Slug algorithm and provides shader sources in GLSL, WGSL, MSL,
  and HLSL.
- Eric Lengyel dedicated the Slug patent to the public domain on `2026-03-17` and published
  reference shaders under an MIT license. This makes a Slug-style in-tree or HarfBuzz-backed vector
  GPU text path realistic for Datoviz without relying on proprietary middleware.
- The commercial Slug Library remains a separate product. This plan is about the now-open algorithm
  and available open reference implementations, not about depending on proprietary Slug middleware.
- MicroTeX is a plausible embeddable C++ library to evaluate for later math layout. It should be
  treated as an optional later lane until the regular text/glyph path is stable.

Useful references:

- HarfBuzz: <https://github.com/harfbuzz/harfbuzz>
- Slug patent/public-domain announcement: <https://terathon.com/blog/decade-slug.html>
- MicroTeX: <https://github.com/NanoMichael/MicroTeX>


## Goals

The text stack should support scientific visualization first, not general desktop publishing.

Required user-facing outcomes:

- high-quality axis labels, tick labels, plot titles, legends, annotations, colorbar labels, and
  pinned readouts;
- glyph markers for scatter-like visuals, categorical symbols, and dense scientific annotations;
- readable text in screen, panel, data, and world coordinate spaces;
- rotated text for axes, labels, and in-scene annotations;
- good Unicode support from UTF-8 inputs;
- deterministic text measurement for layout before rendering;
- GPU-accelerated rendering with stable performance for many labels;
- useful quality/performance choices instead of a single hardcoded renderer;
- DRP2 recording/replay compatibility for text content, font resources, generated atlases, and
  vector text resources;
- a path to minimal LaTeX-style math equations after ordinary text is stable.

Non-goals for the first production slice:

- full document layout;
- rich text editing;
- full TeX/LaTeX compatibility;
- HTML/CSS layout;
- emoji/color-font rendering unless explicitly scoped later;
- automatic global label placement or collision avoidance beyond simple optional heuristics.


## Scientific Visualization Requirements

Text in scientific visualization has stricter placement and measurement requirements than simple UI
labels. The first stable API and implementation should cover the following.

### Text Encodings And Unicode

- Accept UTF-8 strings at the public API boundary.
- Preserve byte strings exactly in retained objects and DVZR recordings.
- Convert to shaped glyph runs through HarfBuzz rather than mapping codepoints directly to glyphs.
- Track grapheme clusters and glyph-to-byte mapping enough for picking, future editing, and precise
  diagnostics.
- Support font fallback per run or glyph when the primary font lacks coverage.
- Support script, direction, and language metadata internally, even if the public API starts with
  auto-detection/defaults.
- Treat combining marks, ligatures, Arabic/Indic shaping, CJK, and right-to-left text as correctness
  requirements for the shaped text path, not as extensions to the ASCII path.

### Metrics And Layout

- Measure text before rendering in deterministic units.
- Expose layout bounds, ink bounds, baseline, ascender, descender, advance, line height, and anchor
  points internally.
- Support single-line layout first, then explicit multiline text, then wrapped text if needed.
- Support horizontal and vertical alignment: left, center, right, baseline, top, middle, bottom.
- Support rotation in screen space and billboard-like world/data placement.
- Support per-label offsets in pixels after coordinate transform.
- Keep point size, pixel size, and data/world units separate.
- Make DPI and framebuffer scale explicit in layout cache keys.

### Coordinate Spaces

Use one placement model across text, annotations, axis labels, colorbar labels, and glyph visuals:

- screen-space: fixed pixel/normalized figure placement;
- panel-space: fixed placement inside a panel viewport;
- data-space: transformed by panel panzoom/arcball/data transform;
- world-space: transformed by camera/controller, optionally billboarding;
- hybrid-space: data/world anchor plus screen-space pixel offset.

The existing `DvzTextPlacement` already points in this direction:

```c
struct DvzTextPlacement
{
    DvzTextPlacementMode mode;
    DvzSceneAnchor anchor;
    double position[3];
    float offset[2];
};
```

The staged implementation should keep this shape if possible, but it may extend it aggressively in
v0.4 because API compatibility is not a constraint on this branch.

### Rendering Behavior

- Respect per-panel viewport and scissor rectangles.
- Respect scene z-layer ordering.
- Support optional depth testing for world/data text.
- Support depth-disabled overlay text for axes, labels, legends, readouts, and UI-like annotations.
- Support alpha blending, premultiplied alpha, and linear/sRGB correctness in the final shader path.
- Avoid text bleeding across panels.
- Avoid accumulating transient per-frame text runtime objects indefinitely.

### Performance

Text must handle both low-count high-quality labels and high-count dense labels:

- cache shaped runs by text, font, style, script/direction/language, and OpenType features;
- cache glyph atlas entries or vector glyph resources by font face, glyph id, render backend, size,
  and renderer-specific parameters;
- reuse GPU buffers for repeated labels when possible;
- support partial updates when only placement changes;
- support partial updates when only string content changes;
- avoid reshaping unchanged text every frame;
- avoid rebuilding atlases unless new glyphs are needed;
- avoid one draw call per label when batching is possible.


## Architecture

The durable architecture should separate text processing from rendering technique.

```text
UTF-8 string + style + placement
  -> font fallback and run segmentation
  -> HarfBuzz shaping
  -> line/box layout
  -> positioned glyph instances
  -> renderer-specific resources
  -> DRP2 buffers/textures/bind groups/draws
```

The shaped/layout output is the common contract. Different renderers can consume that output:

- simple monospace bitmap atlas;
- FreeType bitmap atlas;
- FreeType + SDF/MSDF atlas;
- Slug-style vector GPU resources;
- later math layout that emits ordinary glyph runs plus rule/box primitives.


## Public API Direction

The public API should distinguish three related concepts.

### Font Resources

`DvzFont` should represent a scene-owned font family/face collection, not only one path and size.
Size belongs partly in text style and partly in cache keys.

Potential fields for an expanded descriptor:

```c
typedef struct DvzFontDesc
{
    const char* path;
    const char* family;
    const char* style;
    float default_size_pts;
    uint32_t face_index;
    uint32_t flags;
} DvzFontDesc;
```

First slice can keep the existing fields and add internals only.

### Glyph Visual

The glyph visual is for one glyph/icon/symbol per item. It is closer to point markers than to text
layout.

Use cases:

- categorical scatter markers;
- per-point symbols;
- dense map labels where each item is one short codepoint/glyph;
- icon-like scientific overlays;
- debug glyph rendering.

Potential retained model:

```c
typedef struct DvzGlyphDesc
{
    DvzFont* font;
    const char* glyphs;
    float size_pts;
    uint32_t flags;
} DvzGlyphDesc;
```

Per-item data should include position, glyph id or UTF-8 cluster, color, size, angle, and optional
anchor. For performance, glyph visual should eventually avoid storing per-item strings when glyph ids
are stable.

### Text Visual

The text visual is for strings, runs, labels, and annotations.

Use cases:

- tick labels;
- axis labels;
- plot title;
- annotations;
- legends;
- colorbar labels;
- pinned readouts;
- equation labels once math layout lowers to glyph runs.

Current `DvzText`, `DvzAnnotation`, and `DvzTextStyle` should be refactored toward:

```c
typedef struct DvzTextStyle
{
    DvzFont* font;
    float size_pts;
    uint8_t color[4];
    uint32_t flags;
    bool bold;
    bool italic;
    bool underline;
} DvzTextStyle;
```

Likely future additions:

- font fallback list;
- OpenType features;
- script/direction/language hints;
- horizontal and vertical alignment;
- line spacing;
- max width and wrap policy;
- renderer preference;
- quality/performance hint;
- outline/shadow/background options if needed.


## Renderer Backends

Support multiple text renderers behind one scene API. Renderer choice should be explicit in style or
scene options, with an automatic default once the stack matures.

### Renderer 0: Simple Monospace Atlas

Purpose:

- debug/status overlays;
- quick initial scene integration;
- high-performance low-quality text;
- deterministic tests with tiny fixtures.

Characteristics:

- built-in fixed-size atlas;
- ASCII first, optional Latin-1/limited Unicode later;
- no shaping;
- no font fallback;
- no high-quality scaling;
- one quad per glyph;
- trivial shader.

Why it is still valuable:

- gives the first executable path quickly;
- makes text visible in examples before FreeType/HarfBuzz integration lands;
- avoids blocking text-dependent features like app traces or debug overlays;
- provides a fallback when external dependencies are disabled.

Limitations:

- not acceptable as the main scientific text path;
- not publication quality;
- not enough for Unicode.

### Renderer 1: FreeType Bitmap Atlas

Purpose:

- robust first external-font path;
- hinted small text;
- compatibility fallback for systems where SDF/MSDF or vector paths are disabled.

Characteristics:

- FreeType loads faces and rasterizes glyph bitmaps;
- HarfBuzz shapes text;
- glyph bitmaps are packed into GPU atlases;
- shader samples coverage/alpha texture;
- quality is best near the rasterized size.

Strengths:

- simple, predictable, proven;
- good for small UI/tick text when hinting matters;
- useful fallback for exact visual parity with FreeType metrics.

Weaknesses:

- poor scaling away from atlas size;
- many sizes can create atlas pressure;
- rotated/large text may degrade.

### Renderer 2: SDF/MSDF Atlas

Purpose:

- main first production renderer for scene text.

Characteristics:

- HarfBuzz shapes text;
- FreeType provides outlines/metrics;
- SDF or MSDF glyphs are generated into GPU atlases;
- shader reconstructs smooth coverage at draw time;
- one quad per glyph, batched by font atlas/material.

Strengths:

- good quality over a wider size range than bitmap atlases;
- efficient GPU rendering;
- suitable for axes, labels, legends, annotations, and many screen-space labels;
- implementation is easier to fit into existing DRP2 buffers/textures/draws than vector GPU text.

Weaknesses:

- quality can suffer on tiny hinted text, very sharp corners, large scale factors, and extreme
  perspective;
- atlas generation and packing need careful cache design;
- MSDF generation adds complexity and dependency choices.

Recommended role:

- default production path after simple text;
- target for axes, colorbars, legends, and normal annotations;
- primary path to test font fallback and Unicode shaping.

### Renderer 3: Slug-Style Vector GPU Text

Purpose:

- high-quality scalable vector text;
- large labels;
- rotated text;
- world/data labels viewed at oblique angles;
- publication-grade output;
- future vector graphics synergy.

Characteristics:

- HarfBuzz shapes text;
- glyph outlines are encoded for GPU rasterization;
- shaders evaluate curve coverage directly or through compact curve/band data;
- no size-specific bitmap atlas is required.

Potential implementation paths:

1. evaluate HarfBuzz `libharfbuzz-gpu` as an optional dependency or reference;
2. port/reference the MIT Slug shaders into Datoviz shader assets;
3. build a Datoviz-specific encoder and DRP2 resource contract for curve/band buffers;
4. keep this backend optional until it is stable and validated.

Strengths:

- excellent scaling and rotation quality;
- avoids atlas-size explosion;
- a strong long-term fit for scientific visualization and vector-like overlays.

Weaknesses:

- new runtime resource types and shaders are more complex than atlas quads;
- backend maturity needs evaluation;
- shader portability across Vulkan/WGSL/WebGPU needs real tests;
- CPU-side encoding and cache invalidation need careful design.

Recommended role:

- do not block the first text visual on this path;
- prototype after SDF/MSDF is useful;
- design the shaped-run contract so this backend can plug in without changing public text APIs.


## Internal Components

Introduce internals in `src/scene/` first unless the code becomes broadly useful across modules.
Move cross-module helpers to `src/common` only when multiple active modules need them.

Potential internal files:

- `src/scene/text_font.c`: scene font objects, font loading records, fallback lists.
- `src/scene/text_shape.c`: HarfBuzz shaping, run segmentation, glyph cluster metadata.
- `src/scene/text_layout.c`: line layout, anchors, bounds, placement transforms.
- `src/scene/text_cache.c`: shaped-run and glyph-resource cache keys.
- `src/scene/text_atlas.c`: atlas allocation, eviction, upload planning.
- `src/scene/text_emit.c`: lowering positioned glyphs to DRP2 streams.
- `src/scene/text_backend_simple.c`: simple monospace atlas backend.
- `src/scene/text_backend_sdf.c`: FreeType/SDF/MSDF backend.
- `src/scene/text_backend_vector.c`: Slug-style vector backend prototype.

Potential public headers:

- keep `include/datoviz/scene/text.h` as the scene-facing API;
- add `include/datoviz/text/*.h` only if text becomes an independently reusable module;
- avoid activating an unrelated scaffolding `text` module until the scene slice proves the contract.

Potential tests:

- `src/scene/tests/text.c` for retained text and layout unit tests;
- `src/scene/tests/glyph.c` for glyph visual tests;
- DRP2 fixture coverage for emitted text streams once stable;
- app/offscreen image smoke tests for representative text scenes.


## DRP2 Contract

The first implementation should use existing DRP2 primitives:

- `CreateBuffer` for glyph vertices, instances, uniforms, and index data;
- `WriteBuffer` for dynamic glyph instance data;
- `CreateTexture` and `WriteTexture` for atlas uploads;
- `CreateSampler`, `CreateBindGroupLayout`, `CreateBindGroup`;
- shader modules from scene shader registry;
- render pass draw commands.

Do not add DRP2 text-specific commands for the first slice. Add text-specific DRP2 commands only if
the vector backend proves that generic buffers/textures/draw commands are too awkward or too opaque
for validation and replay.

DRP2 validation should eventually check:

- atlas texture dimensions and usage flags;
- glyph vertex/index buffer sizes;
- bind group layout compatibility;
- shader format availability;
- no draw references destroyed atlas/buffer resources;
- text resources are recreated cleanly on resize or renderer switch;
- DVZR recording has portable enough payloads to replay text scenes.


## FramePlan And Scene Emission

Text should be emitted as a scene visual family or overlay family depending on placement and depth
behavior.

Recommended first policy:

- screen/panel text: overlay render node after ordinary scene visuals;
- data/world text with depth disabled: overlay render node with transformed positions;
- data/world text with depth enabled: normal scene render node in z order;
- glyph visual: normal visual family with configurable depth behavior.

The first slice can use one overlay pass if needed. Later, fold text into existing pass ordering
when the material/blending state model supports it cleanly.

Text emission must respect:

- panel viewport/scissor;
- per-panel controller transforms;
- figure/framebuffer size;
- DPI/framebuffer scale;
- z-layer;
- alpha mode;
- renderer capability checks.


## Caching Model

Use separate cache layers. They should be invalidated independently.

The atlas/cache execution details now live in
[SCENE_TEXT_ATLAS_CACHE_PLAN.md](SCENE_TEXT_ATLAS_CACHE_PLAN.md). The shaped-run details now live in
[SCENE_HARFBUZZ_SHAPING_PLAN.md](SCENE_HARFBUZZ_SHAPING_PLAN.md).

### Font Face Cache

Key:

- font file path or embedded font id;
- face index;
- variation coordinates if supported later;
- font loading flags.

Value:

- FreeType face or equivalent;
- HarfBuzz face/font;
- font metadata;
- available coverage metadata if cheap enough.

### Shaped Run Cache

Key:

- UTF-8 string;
- font fallback list;
- size-independent font style;
- script;
- direction;
- language;
- OpenType features;
- normalization flags if added later.

Value:

- glyph ids;
- cluster ids;
- advances and offsets in font units;
- run segmentation metadata;
- selected font face per run.

Do not key shaped runs by placement or transform.

### Layout Cache

Key:

- shaped run id;
- size in points/pixels;
- DPI/framebuffer scale;
- line spacing;
- max width/wrap policy;
- alignment/anchor;
- renderer metrics mode.

Value:

- positioned glyph instances in local label coordinates;
- layout bounds and ink bounds;
- baseline and anchor offsets.

### Renderer Resource Cache

Key:

- font face;
- glyph id;
- renderer backend;
- size bucket for bitmap paths;
- SDF/MSDF parameters;
- vector encoder version for Slug-style paths.

Value:

- atlas slot and UVs;
- or vector curve/band buffer offsets;
- resource generation/version metadata.


## Font Fallback

Font fallback is required for good Unicode support.

First fallback strategy:

1. primary `DvzFont`;
2. scene default font;
3. platform default sans font if available;
4. bundled fallback font if the project adds one;
5. missing-glyph box as a final explicit fallback.

Later fallback improvements:

- user-provided fallback chain;
- script-aware fallback;
- emoji/color-font policy;
- CJK fallback policy;
- cache coverage maps per face;
- warning diagnostics for missing glyphs.

Keep fallback deterministic. The same input and configured fonts should produce the same glyph runs
across runs when possible.


## Dependency Strategy

### FreeType

Use FreeType for:

- font loading;
- face selection;
- glyph metrics;
- hinted rasterization in the bitmap path;
- outlines for SDF/MSDF generation if needed.

Treat it as the first external dependency to add for production text.

### HarfBuzz

Use HarfBuzz for:

- text shaping;
- glyph ids and advances;
- OpenType features;
- complex script correctness;
- future integration with `libharfbuzz-gpu`.

Do not hand-roll shaping beyond the simple debug renderer.

### ICU/Unicode Helpers

Evaluate only when needed. HarfBuzz can shape, but full Unicode segmentation, BiDi handling, locale,
normalization, and line breaking may require extra helpers.

Possible staged approach:

- first: HarfBuzz defaults plus explicit direction/script/language fields where available;
- second: small Unicode helpers for UTF-8 validation and grapheme iteration;
- third: ICU or another library if BiDi, line breaking, and locale-sensitive behavior become
  first-class requirements.

### MSDF Generation

Evaluate whether to vendor or implement a narrow generator.

Requirements:

- deterministic glyph images for tests;
- acceptable build complexity;
- C or C-compatible integration where possible;
- safe handling of malformed fonts;
- cacheable output.

### Slug/HarfBuzz GPU

Treat Slug-style rendering as a prototype lane after SDF/MSDF. The first investigation should answer:

- can HarfBuzz `libharfbuzz-gpu` produce resource data that maps cleanly to DRP2 buffers?
- are GLSL/WGSL shaders usable in the existing scene shader registry?
- what are the resource lifetime and cache keys?
- how does quality compare against MSDF for rotated, large, and oblique text?
- does it work for all shaped glyph runs we care about, or only simple outlines?


## Math And Equation Support

Math support should be built on top of the shaped text and renderer backends, not as a separate image
renderer by default.

### Minimal Equation Target

The first math path should support:

- inline equations in labels;
- display equations as annotations;
- Greek letters and common symbols;
- superscripts and subscripts;
- fractions;
- square roots;
- sums, integrals, products;
- parentheses/brackets with simple stretch behavior;
- basic matrices later, not first.

Examples:

```text
alpha = 0.05
E = mc^2
\sigma = \sqrt{\frac{1}{N}\sum_i (x_i - \mu)^2}
f(x) = \int_0^\infty e^{-x^2} dx
```

### MicroTeX Evaluation

MicroTeX is a plausible candidate for math parsing/layout. Evaluate it only after regular text is
usable.

Questions to answer:

- license compatibility;
- build system fit with CMake and Datoviz C/C++ conventions;
- whether it can expose layout boxes without forcing a CPU bitmap output path;
- whether glyph selection can be routed through Datoviz fonts or bundled math fonts;
- how much of LaTeX math syntax it supports;
- how robust it is on malformed user strings;
- how expensive layout is for dynamic labels.

### Lowering Model

Preferred model:

```text
LaTeX-like equation
  -> math parser/layout tree
  -> positioned glyph runs + vector rules/boxes
  -> normal Datoviz text/glyph/primitive rendering
```

Avoid making equation support only "render LaTeX to PNG, upload texture" unless used as a temporary
prototype. Texture-only equations can be useful as a fallback, but they will not share selection,
scaling, color, vector quality, or DVZR semantics as cleanly as glyph-run lowering.


## Staged Implementation Plan

### Stage 0: Requirements, API, And Fixtures

Scope:

- audit current `DvzFont`, `DvzText`, `DvzAnnotation`, `DvzTextStyle`, and placement structs;
- define the internal shaped-run and layout structs;
- define renderer enum and renderer capability flags;
- define scene diagnostics for missing fonts, missing glyphs, and unsupported renderer features;
- add tests for retained object lifecycle and style/placement updates;
- add one placeholder scene text fixture that validates no-op/bookkeeping behavior.

Deliverables:

- planning-backed header/API patch;
- internal struct definitions;
- focused tests that do not require FreeType/HarfBuzz yet.

Validation:

- `just build`
- `just test scene`
- `git diff --check`

### Stage 1: Simple Monospace Renderer

Scope:

- add a tiny built-in monospace atlas or generated atlas texture;
- support ASCII text strings;
- lower text into glyph quads;
- emit atlas texture, sampler, vertex/instance buffers, bind groups, and draw calls through DRP2;
- render screen/panel-space text in an overlay pass;
- add one app/offscreen visual smoke test if practical.

Deliverables:

- first visible `DvzText`;
- first visible retained annotation labels;
- simple shader path in scene shader registry;
- tests for panel scissor, placement offset, and repeated frame reuse.

Validation:

- `just build`
- focused `dvztest_scene` text filters
- `just test scene`
- one offscreen example or screenshot smoke
- `git diff --check`

### Stage 2: Glyph Visual

Scope:

- add a retained glyph visual family for per-item symbols;
- reuse the simple atlas backend first;
- support per-item position, color, size, angle, and glyph index/character;
- support data and screen placement modes;
- batch glyph instances by atlas/material.

Deliverables:

- glyph visual API;
- visual data update path;
- scene emission tests;
- one example with glyph markers.

Validation:

- `just build`
- focused scene glyph tests
- `just test scene`
- app/offscreen smoke if renderer output is stable enough
- `git diff --check`

### Stage 3: FreeType Font Loading

Scope:

- add optional FreeType dependency and CMake feature flag;
- load scene-owned font files;
- extract metrics and glyph bitmaps;
- keep font face lifetime scene-scoped or cache-scoped;
- add missing-font diagnostics and fallback behavior.

Deliverables:

- real font loading behind `DvzFont`;
- bitmap atlas renderer for shaped glyph ids once Stage 4 lands;
- tests using a known vendored or test font asset.

Validation:

- `just build`
- focused scene font tests
- `git diff --check`
- static analysis on touched C/C++ files when practical.

### Stage 4: HarfBuzz Shaping

Scope:

- add optional HarfBuzz dependency and CMake feature flag;
- shape UTF-8 strings into glyph runs;
- store glyph ids, clusters, advances, offsets, directions, and selected font face;
- add first font fallback chain;
- add layout measurement for single-line text.

Deliverables:

- Unicode-capable shaped text path;
- tests for ligatures, combining marks, right-to-left text, CJK fallback, and missing glyphs;
- diagnostics for unsupported shaping when HarfBuzz is disabled.

Validation:

- `just build`
- focused shaping/layout tests
- `just test scene`
- `git diff --check`

### Stage 5: Bitmap Atlas Production Path

Scope:

- connect FreeType rasterization and HarfBuzz shaped glyph ids;
- pack glyph bitmaps into atlas textures;
- upload atlas updates through DRP2;
- render shaped text with correct advances and offsets;
- cache shaped runs, layout, and atlas slots.

Deliverables:

- readable high-quality small text;
- labels with real fonts;
- font fallback for missing glyphs;
- retained text update tests that avoid rebuilding unchanged resources.

Validation:

- `just build`
- focused text renderer tests
- `just test scene`
- app/offscreen smoke with labels and annotations
- `git diff --check`

### Stage 6: SDF/MSDF Renderer

Scope:

- generate SDF or MSDF atlas glyphs from font outlines;
- add renderer-specific shader;
- add quality parameters and renderer selection;
- compare quality against bitmap atlas for small, large, rotated, and scaled text;
- retain bitmap atlas as fallback.

Deliverables:

- default production text backend candidate;
- renderer option in style/scene config;
- visual examples for axes, labels, colorbars, and annotations.

Validation:

- `just build`
- focused SDF/MSDF tests
- `just test scene`
- offscreen screenshot smoke
- validation-layer smoke for Vulkan path if shader/resource changes are nontrivial
- `git diff --check`

### Stage 7: Axes, Colorbars, Legends, And Readouts

Scope:

- integrate text with native 2D/3D axes once the axis API lands;
- render tick labels, axis labels, titles, and colorbar titles/ticks;
- use text measurement for layout;
- support rotated axis labels;
- connect pinned readouts and annotations to the same renderer;
- add formatting integration with existing `DvzFormatDesc`.

Deliverables:

- first complete scientific text workflow;
- examples with axis labels, tick labels, colorbar labels, annotations, and pinned readouts;
- tests for resize/DPI behavior and panel clipping.

Validation:

- `just build`
- focused scene axes/text tests
- `just test scene`
- representative examples
- `git diff --check`

### Stage 8: Unicode Hardening

Scope:

- improve fallback selection;
- add script/direction/language controls;
- add BiDi and grapheme-cluster helpers if needed;
- add multiline layout;
- add text measurement API if useful publicly;
- add robust diagnostics for missing fonts/glyphs and malformed UTF-8.

Deliverables:

- expanded Unicode regression fixtures;
- more deterministic fallback behavior;
- documented limitations for emoji/color fonts and vertical text if still unsupported.

Validation:

- `just build`
- focused Unicode text tests
- `just test scene`
- `git diff --check`

### Stage 9: Slug-Style Vector GPU Prototype

Scope:

- evaluate HarfBuzz `libharfbuzz-gpu` and the MIT Slug reference shaders;
- define Datoviz vector glyph resource cache;
- map curve/band data to DRP2 buffers;
- add shader variants for Vulkan first, WGSL/WebGPU later if practical;
- compare output and performance against MSDF.

Deliverables:

- optional vector text backend prototype;
- quality/performance report;
- decision on whether to productionize the backend in v0.4.

Validation:

- `just build`
- focused vector text tests
- offscreen screenshot comparisons
- validation-layer Vulkan smoke
- `git diff --check`

### Stage 10: Minimal Math/Equation Support

Scope:

- evaluate MicroTeX;
- define equation retained object or text style mode;
- lower math boxes to glyph runs plus simple rule/box primitives;
- support inline and display equations;
- add math font/fallback policy.

Deliverables:

- first equation labels in annotations and axes;
- examples with common scientific equations;
- documented supported subset.

Validation:

- `just build`
- focused math layout tests
- scene text/equation rendering tests
- representative offscreen examples
- `git diff --check`


## Example Milestone API Sketch

The exact public API can change, but a plausible milestone target is:

```c
DvzFont* font = dvz_font(
    scene, &(DvzFontDesc){
               .path = "fonts/Inter-Regular.ttf",
               .default_size_pts = 12.0f,
           });

DvzText* title = dvz_text(
    panel, &(DvzTextDesc){
               .string = "Power spectrum: P(k)",
               .style = {
                   .font = font,
                   .size_pts = 14.0f,
                   .color = {255, 255, 255, 255},
               },
               .placement = {
                   .mode = DVZ_TEXT_PLACEMENT_SCREEN,
                   .anchor = DVZ_SCENE_ANCHOR_PANEL_TOP,
                   .offset = {0.0f, 8.0f},
               },
           });

dvz_text_set_string(title, "Power spectrum: P(k), z = 2.0");
```

Later equation shape:

```c
DvzText* eq = dvz_text(
    panel, &(DvzTextDesc){
               .string = "\\sigma = \\sqrt{\\frac{1}{N}\\sum_i (x_i - \\mu)^2}",
               .style = {
                   .font = math_font,
                   .size_pts = 13.0f,
                   .flags = DVZ_TEXT_STYLE_LATEX_MATH,
               },
               .placement = {
                   .mode = DVZ_TEXT_PLACEMENT_DATA,
                   .anchor = DVZ_SCENE_ANCHOR_DATA,
                   .position = {0.5, 0.8, 0.0},
                   .offset = {8.0f, -8.0f},
               },
           });
```


## Testing Strategy

Unit tests:

- UTF-8 validation and error handling;
- font object lifecycle;
- fallback chain selection;
- shaping output metadata;
- layout bounds and anchors;
- atlas allocation and eviction;
- resource cache invalidation.

Scene emission tests:

- text emits expected DRP2 resource commands;
- repeated frame does not recreate unchanged resources unnecessarily;
- string update updates shaped/layout data;
- placement-only update avoids reshaping and atlas rebuild;
- resize/DPI changes invalidate layout but not shaped runs;
- panel scissor prevents cross-panel text bleeding;
- destroyed text/font resources are not emitted.

Rendering smoke tests:

- simple overlay text;
- rotated axis label;
- data-anchored label with panzoom;
- world label with arcball scene;
- dense glyph visual;
- Unicode sample;
- equation sample once math lands.

Runtime/recording tests:

- DVZR records and replays text scenes;
- atlas payloads are portable enough for semantic replay;
- vector backend resources are either portable or explicitly diagnosed;
- app/offscreen capture contains text after replay.


## Risks And Open Questions

- Dependency size and packaging: FreeType and HarfBuzz add build complexity. Keep feature flags and
  clear diagnostics when disabled.
- Font assets: tests need deterministic fonts. Decide whether to vendor a small permissive test font.
- Fallback determinism: platform fallback can vary. Prefer explicit fallback fonts for tests.
- Atlas pressure: many sizes, scripts, and dynamic labels can fragment atlases. Cache metrics and
  eviction policy need tests.
- MSDF dependency choice: implementation quality matters. A poor MSDF generator will create visible
  artifacts.
- Slug-style backend maturity: the algorithm is now available, but Datoviz still needs a robust
  encoder, resource model, and shader integration.
- WebGPU/WGSL parity: text shaders should not assume Vulkan-only semantics if WebGPU remains on the
  roadmap.
- Math fonts: minimal equation support may require bundled math fonts or explicit user font setup.
- Color fonts and emoji: defer unless required; document limitations.
- BiDi and line breaking: HarfBuzz shapes runs but does not solve every paragraph-layout problem by
  itself.


## Recommended First Implementation Path

Implement in this order:

1. refactor text/glyph internal contracts and tests without external dependencies;
2. render simple monospace text through scene -> DRP2 -> vklite;
3. add glyph visual on the same simple renderer;
4. add FreeType and HarfBuzz behind feature flags;
5. add bitmap atlas shaped text;
6. add SDF/MSDF as the default production renderer;
7. integrate axes, colorbars, legends, and annotations;
8. harden Unicode/fallback behavior;
9. prototype Slug-style vector GPU text;
10. evaluate MicroTeX and implement minimal equation lowering.

This keeps visible progress early while preserving a path to high-quality Unicode, GPU-accelerated
scientific text, and later equations without changing the public scene model every time the renderer
backend changes.
