> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-09`
> - **Purpose:** define the intended v0.4 text architecture, including world-space text,
>   screen-space annotations, font/atlas resources, and equation-backend integration.

# Text Design

This note records the rationale and forward-looking design details behind the v0.4 text
architecture.

The normative text contract now lives in
[../semantics/TEXT.md](../semantics/TEXT.md). Keep this proposal as background for future
implementation work, equation-backend integration, and lower-level rendering choices that are not
yet implementation-ready.


## Objective

Support text as a first-class scene feature, including:

1. screen-space annotation text,
2. world-space 3D text,
3. simple equation/math rendering,
4. shared font and glyph resources,
5. per-glyph color and minimal style/decorations,
6. DPI-aware rendering,
7. a backend-agnostic design that can evolve beyond the first implementation.


## Why This Is Active Now

Text is not a later decorative feature.

It is already needed for:

1. world-space labels,
2. measurement annotations and dimensions,
3. adaptive scale bars,
4. future axes and figure labels,
5. simple equation rendering,
6. UI overlays that must stay crisp under DPI scaling.

The architecture should be defined now so mesh, annotation, picking, and resource update work do
not harden around the wrong assumptions.


## v0.3 Assessment

v0.3 already had useful text-related pieces:

1. glyph-based visuals in
   [v0.3/src/scene/visuals/glyph.c](../../../v0.3/src/scene/visuals/glyph.c)
2. atlas/font helpers in
   [v0.3/include/datoviz/scene/atlas.h](../../../v0.3/include/datoviz/scene/atlas.h)
3. font metrics/loading support in `v0.3/src/scene/font.c`
4. world-positioned glyph placement through per-glyph `position`, `axis`, `anchor`, `shift`,
   `group_size`, and related attributes
5. MSDF-style shader support in
   `v0.3/include/datoviz/scene/glsl/text_functions.glsl`

What v0.3 got right:

1. text was treated as renderable geometry/visual data, not only CPU overlays,
2. world-space placement already existed,
3. atlas-backed glyph rendering was practical,
4. grouped strings and anchor logic were useful for labels.

What should change in v0.4:

1. avoid exposing overly low-level per-glyph plumbing as the primary public API,
2. separate shaping/layout from rendering resources,
3. define reusable scene-owned font/atlas resources,
4. support both screen-space and world-space text explicitly,
5. leave room for equation backends and future GPU outline rendering paths.


## Core Architecture Split

The text system should be split into distinct layers:

1. shaping/layout,
2. font loading and metrics,
3. glyph resource generation,
4. text visual rendering,
5. optional equation backend integration.

These layers should not be conflated.


## Recommended Baseline Stack

Recommended first practical stack:

1. FreeType for font loading and glyph metrics/outlines,
2. HarfBuzz for shaping,
3. atlas-backed glyph rendering as the first runtime path.

Why:

1. it is practical now,
2. it supports both simple labels and more complex shaping if needed,
3. it works for both 2D and 3D text placement,
4. it leaves room for future backend replacement.


## Text Resource Model

Text should use explicit scene-owned resources, consistent with the broader scene direction.

Recommended resource families:

1. font resources,
2. atlas resources,
3. optional text-style resources later,
4. text visual instances that borrow font/atlas resources.

Suggested conceptual ownership:

1. `DvzFontResource`
   - font file/bytes
   - metrics source
   - shaping-facing identity
2. `DvzGlyphAtlasResource`
   - atlas texture
   - glyph metadata / UV mapping
   - region-growth and upload tracking
3. text visual
   - references font/atlas resources
   - owns placement, styling, alignment, transform, and content binding state

Do not make each text object or glyph visual own its own private atlas by default.


## Update Model

Text depends on partial-update infrastructure from day one.

Phase-1 text requirements:

1. atlas texture region uploads,
2. retained string/glyph-run updates,
3. geometry subrange updates for text quads when content changes,
4. explicit dirty tracking for newly added glyphs and changed strings,
5. DPI-driven atlas rebuild or glyph-patch behavior when physical resolution changes.

This is one reason the scene resource-update contract is important now.


## Public Text Concepts

The text API should expose higher-level concepts than the old per-glyph low-level v0.3 entry
points.

Recommended public concepts:

1. text visual,
2. text run / string content,
3. text style,
4. world-space or screen-space placement mode,
5. alignment / anchor,
6. scale mode,
7. per-run or per-glyph color policy,
8. minimal style/decorations,
9. optional equation content source.

The public API should not require most callers to manually set per-glyph UVs, shifts, and atlas
coordinates.


## Color Model

Text needs more than one flat run color.

Recommended baseline support:

1. one default color per text run,
2. optional per-glyph color override,
3. alpha participates normally in transparency/render-mode rules.

Why:

1. equations and annotations often need emphasis within one run,
2. scientific overlays may need per-glyph semantic coloring,
3. this should be supported without forcing one visual per color change.

Recommendation:

1. the common path remains run-level color,
2. per-glyph color is supported as an optional richer content path,
3. the public API should not force every simple label through per-glyph payloads.


## Color Fonts And Emoji

Color-glyph support should be acknowledged now.

Recommended split:

1. the baseline text path is monochrome/SDF/MSDF-style glyph rendering,
2. color fonts and emoji use a distinct glyph-rendering path when the font exposes color glyphs,
3. the text API remains the same at the content level.

Why:

1. color emoji are not well represented by the same assumptions as monochrome atlas text,
2. scientific apps may still want Unicode-rich labels or symbols,
3. this should not distort the baseline text architecture.

Recommendation:

1. do not make color emoji the baseline rendering path,
2. do reserve a color-glyph fallback/backend path in the design,
3. treat color glyphs as an atlas/backend capability question, not a separate public text family.


## 2D and 3D Text

Text should explicitly support both:

1. screen-space annotation text,
2. world-space 3D text.

These are distinct placement modes and should be represented as such.


## Screen-Space Text

Screen-space text is needed for:

1. axes and labels,
2. figure annotations,
3. scale bars,
4. HUD-style overlays.

Recommended behavior:

1. placement in panel or figure screen coordinates,
2. text size defined in pixels or screen-relative units,
3. unaffected by model-space arcball unless explicitly requested.

Recommended DPI rule:

1. text sizing and placement should remain expressed in logical units at the scene boundary,
2. runtime DPI scaling should realize those logical units without changing the semantic text API,
3. glyph atlas rebuild or patch behavior remains an internal/runtime consequence of DPI changes.


## World-Space 3D Text

World-space text is required now and should be first-class.

Recommended behavior:

1. anchored at a 3D world/object position,
2. optional billboard or fixed-orientation modes,
3. explicit size mode:
   - world units
   - screen/pixel-scaled
4. configurable depth behavior later:
   - test/write modes as needed for labels and annotations.

This should not be treated as a hack layered on top of screen-space glyphs.


## Picking Scope

The first text slice should keep picking intentionally coarse.

Recommended first behavior:

1. text picking resolves at the string or text-object level,
2. sub-string or glyph-level picking is deferred,
3. text selection/highlight should still integrate with the shared scene interaction model.


## Resource Boundary

Text resources should stay semantically separated from glyph-atlas runtime details.

Recommended rule:

1. text content, style, and placement are semantic scene concepts,
2. atlas pages, UVs, and glyph packing remain runtime-resource details,
3. export, selection, and readout should refer back to semantic text objects rather than atlas
   internals.


## Orientation Modes

World-space text will need explicit orientation behavior.

Recommended initial modes to reserve:

1. screen-aligned billboard,
2. world-oriented text plane,
3. later axis-aligned or custom-basis modes if needed.

Do not bury orientation semantics in ad hoc transform conventions.


## Text Scale Modes

Text scale should be explicit because both scientific labels and world-space dimensions need
different behavior.

Recommended modes:

1. screen/pixel size,
2. world-unit size.

Why:

1. labels sometimes need constant on-screen legibility,
2. measurement/dimension annotations sometimes need world-relative scale semantics.


## Content Model

The text system should support content as runs or strings, not only raw glyph arrays.

Recommended content levels:

1. plain text string,
2. shaped glyph run,
3. equation/layout display list from a backend.

This lets the public API stay high-level while still allowing advanced callers to bypass shaping
when appropriate.


## Shaping and Script Handling

Recommended rule:

1. text shaping should be a dedicated step,
2. public text APIs should not assume ASCII-only forever,
3. plain single-line labels should remain easy to use,
4. style runs should be allowed to influence shaping when a font/style system requires it.

Even if many scientific labels are simple, shaping should not be designed out of the system.


## Equation Backend Direction

The text system should leave room for a simple equation backend now.

Recommended direction:

1. do not implement full TeX layout inside Datoviz,
2. allow an external/frontend backend to emit a structured composition,
3. support composition primitives needed by equations:
   - glyph runs
   - rules/lines
   - rectangles/backgrounds
   - transforms/positioning

This is compatible with a MicroTeX-style frontend if it can emit:

1. glyph ids,
2. glyph placements,
3. rules,
4. boxes/background rectangles.

The key boundary is: the equation backend should produce a display list or structured text/shape
composition that the scene text/annotation layer can render.


## Minimal Style And Decoration Support

Minimal style support is needed now, but it should stay narrow.

Recommended baseline:

1. regular
2. bold
3. italic
4. underline

Recommendation:

1. bold and italic should be style-run properties,
2. underline should be a derived decoration primitive, not a separate font requirement,
3. do not overbuild rich-text editing or CSS-like styling.

This is enough for labels, equations, and measurement annotations without turning text into a full
document system.


## Font Choices and Bundled Assets

The architecture should not assume one hardcoded font, but it should support curated bundled fonts
for predictable scientific output.

Examples mentioned for future use:

1. NewCMMath-Regular.otf for equation-oriented workflows

Recommendation:

1. font resources should be generic,
2. bundled fonts should be treated as convenience assets, not architectural assumptions.


## DPI And UI Scaling

High-DPI behavior should be explicit in the text design rather than left to implementation folklore.

Relevant broader context:

1. [spec/scene/integration/HIGH_DPI.md](../integration/HIGH_DPI.md)

Recommended policy:

1. scene-facing text sizes remain expressed in logical pixels or world units, depending on scale mode,
2. raster/atlas generation uses physical resolution derived from current `dpi_scale`,
3. DPI changes mark dependent text resources dirty,
4. screen-space text, annotation labels, and UI-adjacent overlays should remain visually crisp under
   DPI changes.

This is important not only for labels but also for future external-UI integration.


## Backend-Agnostic Rendering Direction

The first implementation should likely be atlas-backed, but the API should not hardcode that as the
only future rendering model.

One plausible future or optional backend is Slug-like direct GPU font rendering from outlines.

Useful references:

1. [Slug Library](https://sluglibrary.com/)
2. [Hackaday, March 20, 2026: Slug Algorithm Now In Public Domain](https://hackaday.com/2026/03/20/slug-algorithm-for-on-gpu-rendering-of-fonts-with-bezier-curves-now-in-public-domain/)

Recommendation:

1. use atlas-backed rendering first,
2. keep the text resource and visual API backend-agnostic enough that a direct-GPU-outline backend
   can slot in later.


## Relationship To Measurement and Annotation

Text should be designed with annotation use cases in mind from the start.

Important downstream consumers:

1. adaptive scale bars,
2. world-space dimension labels,
3. 3D bounding-box annotations,
4. axis labels and tick labels.

This means the text API should be comfortable with:

1. single-line labels,
2. rotated labels,
3. mixed screen-space and world-space placement,
4. small structured compositions rather than full paragraph layout.

Complex paragraph layout is not a Phase-1 requirement.


## Picking and Interactivity

Text visuals should participate in the picking model later, but text picking does not need to lead
the first implementation.

Recommended direction:

1. object-level picking support for text visuals should be possible,
2. glyph-level picking is not a Phase-1 requirement,
3. label hit testing can be approximate or deferred until a real workflow needs it.


## Recommended First Visual Family Shape

A practical first v0.4 text surface should expose one or both of:

1. a high-level retained text object for common labels,
2. an annotation-oriented object that can consume text plus simple geometric primitives.

The simpler path is:

1. create a retained semantic `DvzText` object that lowers to `glyph` visual contributions,
2. give it high-level content/style/placement setters,
3. let future equation and measurement layers build on top of it.


## Initial Public API Direction

The exact names can still change, but the intended public shape should look something like:

1. create font resource,
2. create atlas resource,
3. create retained text object,
4. set content or shaped run,
5. set placement mode,
6. set anchor/alignment,
7. set scale mode,
8. attach to panel.

Suggested conceptual calls:

1. `DvzFont* dvz_font(...)`
2. `DvzText* dvz_text(DvzScene* scene, const DvzTextDesc* desc);`
3. `int dvz_text_set_string(...)`
4. `int dvz_text_set_world_position(...)`
5. `int dvz_text_set_screen_position(...)`
6. `int dvz_text_set_style(...)`
7. `int dvz_text_set_equation(...)` or equivalent later

The exact surface can evolve, but the content/placement/style split should remain clear.


## Explicit Non-Goals For The First Text Slice

1. full paragraph layout engine,
2. full TeX implementation inside Datoviz,
3. backend lock-in to only one atlas implementation,
4. glyph-level interaction semantics,
5. every possible text decoration or rich-text feature.


## Recommended Immediate Scope

The first useful text implementation should aim for:

1. explicit font and atlas resources,
2. atlas-backed glyph rendering,
3. screen-space single-line labels,
4. world-space 3D labels,
5. anchor/alignment support,
6. screen-size versus world-size scaling,
7. run-level color with optional per-glyph color support,
8. minimal style/decorations,
9. DPI-aware atlas behavior,
10. text content updates compatible with retained scene resources,
11. room for later equation display-list integration.
