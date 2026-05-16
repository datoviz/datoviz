# Datoviz v0.4 Text, Glyph, and LaTeX/Math Rendering Specification

## Goal

Add a bundled text and math rendering subsystem to Datoviz v0.4 that supports:

- arbitrary `.ttf` / `.otf` fonts;
- high-quality antialiased glyph rendering through SDF/MSDF/MTSDF or, later, vector paths;
- screen-space and world-space text placement;
- mathematical notation using a bundled MicroTeX-like engine;
- no runtime dependency on an external LaTeX distribution, TeX Live, MiKTeX, `latex`, `pdflatex`, `dvipng`, or system-installed fonts.

The user should be able to install Datoviz and render labels, Unicode text, and common LaTeX-style math expressions out of the box.

---

## Scope

This specification targets Datoviz v0.4 and the new scene/DRP architecture.

The exact public C/Python API is intentionally left flexible. The purpose is to define the architecture, internal data flow, implementation phases, and acceptance criteria.

The system should support two related but distinct visual types:

1. **Text / Glyph Visual**
   - Generic Unicode text.
   - Arbitrary fonts.
   - Per-glyph positioning.
   - SDF/MSDF atlas rendering.

2. **Math / Equation Visual**
   - LaTeX-style mathematical expressions.
   - Initially rendered as bitmap/image fallback.
   - Eventually rendered as glyph boxes through the same glyph visual.

---

## Non-goals

The first implementation should not attempt to provide a full LaTeX document engine.

Out of scope for the initial version:

- arbitrary LaTeX documents;
- page layout;
- TikZ;
- macros requiring a full TeX engine;
- external package loading;
- system LaTeX discovery;
- PDF generation;
- publication-grade TeX compatibility.

The target is closer to **math-mode LaTeX subset rendering**, similar in spirit to Matplotlib mathtext, KaTeX, or MathJax math rendering.

---

## Architectural principle

The text and math system should be layered:

```text
UTF-8 text / LaTeX math string
        ↓
font resolution / math parsing / shaping / layout
        ↓
glyph runs, glyph boxes, or bitmap fallback
        ↓
Datoviz scene resources
        ↓
DRP buffers, textures, samplers, shaders, pipelines
        ↓
backend rendering: Vulkan / WebGPU / headless / video
```

The scene layer should own:

- text layout;
- math layout;
- font fallback;
- glyph atlas allocation;
- dirty tracking;
- visual-local text state;
- conversion to DRP resources.

The GPU/backend layer should only see normal rendering primitives:

- buffers;
- textures;
- texture views;
- samplers;
- shader modules;
- render pipelines;
- draw commands.

---

## Recommended dependency stack

### Required or strongly recommended

#### FreeType

Use FreeType for:

- loading `.ttf` / `.otf` font files;
- reading glyph outlines;
- extracting glyph metrics;
- optional raster glyph fallback;
- accessing font faces from bundled assets or user-provided paths.

#### HarfBuzz

Use HarfBuzz for:

- shaping Unicode text;
- glyph clustering;
- ligatures;
- kerning;
- combining marks;
- right-to-left and complex scripts, when supported by the selected fonts.

Even if math is the primary target, HarfBuzz is important for a correct general-purpose text visual.

#### MSDF generation

Use an MSDF/MTSDF generation library, such as `msdfgen`, or an equivalent internal module, for:

- generating signed-distance glyph atlas entries from font outlines;
- high-quality antialiased glyph rendering;
- scale-independent rendering over a useful size range.

Initial implementation may use grayscale SDF or raster alpha atlas if MSDF integration is delayed, but the visual and atlas architecture should be designed for MSDF/MTSDF.

#### MicroTeX or equivalent

Use a bundled MicroTeX-like library for:

- parsing LaTeX math strings;
- building equation layout boxes;
- rendering equations without requiring an external LaTeX installation.

The preferred long-term integration is structured output:

```text
LaTeX expression → box tree → glyph boxes → Datoviz glyph instances
```

The first milestone may use bitmap output:

```text
LaTeX expression → CPU RGBA bitmap → Datoviz image visual
```

---

## Bundled assets

Datoviz binary distributions should include a minimal default font bundle.

Recommended candidates:

- UI/sans font: Inter, Noto Sans, or similar permissively licensed font;
- monospace font: Noto Sans Mono, JetBrains Mono, or similar;
- math font: Latin Modern Math, STIX Two Math, or Libertinus Math;
- optional symbol fallback font for mathematical symbols and Unicode coverage.

The chosen fonts must have licenses compatible with Datoviz distribution, ideally SIL Open Font License or similarly permissive terms.

The build should not rely on system fonts for basic functionality.

---

## Main visual types

## 1. GlyphVisual / TextVisual

The base visual should render one quad per glyph, usually instanced.

### CPU-side responsibilities

The text subsystem should transform a text string into glyph instances:

```text
UTF-8 string
→ font fallback and shaping
→ glyph IDs and glyph metrics
→ glyph atlas lookup/allocation
→ glyph instance records
→ scene resources marked dirty
```

### GPU-side representation

Each glyph instance should contain at least:

```c
struct GlyphInstance {
    vec3 position;       // anchor position in panel/world coordinates
    vec2 offset;         // glyph offset from text origin
    vec2 size;           // glyph quad size
    vec4 color;          // RGBA
    vec4 uv;             // atlas rectangle: u0, v0, u1, v1
    float angle;         // optional rotation
    uint32_t flags;      // coordinate mode, decoration, etc.
};
```

This is illustrative. The final memory layout may differ.

### Coordinate modes

The visual should support at least:

- **screen/pixel space**: labels, annotations, overlays, axes;
- **panel normalized space**: titles, legends, HUD text;
- **data/world space**: scientific labels attached to plotted objects;
- **mixed mode**: world-space anchor with pixel-space glyph size.

The mixed mode is especially useful:

```text
anchor follows a data/world coordinate
text size remains constant in screen pixels
```

### Text layout features

Initial required features:

- single-line layout;
- multi-line layout;
- horizontal alignment: left, center, right;
- vertical alignment: top, center, baseline, bottom;
- font size in pixels or points;
- color;
- opacity;
- rotation;
- DPI scaling;
- atlas cache reuse.

Later features:

- rich text spans;
- per-span font and color;
- fallback fonts;
- text wrapping;
- ellipsis;
- underline/strikethrough;
- selection/picking.

---

## 2. MathVisual / EquationVisual

The math visual should be a higher-level frontend that eventually emits either:

1. image visual data, or
2. glyph visual instances.

The public abstraction should not expose implementation details.

A user should write something like:

```python
panel.math(r"\nabla \cdot \mathbf{E} = \rho / \varepsilon_0", pos=(0.5, 0.9))
```

or, in C-like form:

```c
DvzVisual* eq = dvz_math(panel);
dvz_math_latex(eq, "\\int_0^\\infty e^{-x^2}\\,dx = \\frac{\\sqrt\\pi}{2}");
dvz_math_position(eq, x, y);
```

The exact names are placeholders.

### Math rendering modes

#### Mode A: bitmap fallback

First implementation target.

```text
LaTeX math string
→ MicroTeX parses and lays out equation
→ CPU RGBA or alpha bitmap
→ Datoviz texture resource
→ Image visual or textured quad visual
```

Advantages:

- fastest route to usable math rendering;
- isolates MicroTeX complexity;
- robust for common labels, titles, and annotations;
- avoids implementing math glyph extraction immediately.

Drawbacks:

- resolution dependent;
- scaling artifacts unless rerendered at target scale/DPI;
- less flexible for per-glyph color, picking, and 3D placement;
- less elegant than direct glyph rendering.

#### Mode B: glyph box rendering

Long-term target.

```text
LaTeX math string
→ MicroTeX box tree
→ glyph boxes with font, glyph ID, position, scale
→ Datoviz glyph atlas
→ GlyphVisual instances
```

Advantages:

- sharp at arbitrary scale;
- integrates with text visual;
- allows per-glyph styling;
- reusable atlas;
- better for world-space annotations.

Drawbacks:

- requires structured access to MicroTeX layout;
- requires symbol-to-font mapping;
- needs robust baseline and metric handling;
- stretchable delimiters, radicals, accents, and large operators are hard.

The public API should allow the backend mode to change internally without breaking user code.

---

## Font and atlas management

Introduce a scene-level font manager.

Suggested internal components:

```text
font_manager
font_face
glyph_cache
glyph_atlas
text_layout
math_layout
microtex_bridge
text_visual
math_visual
```

### Font manager

Responsibilities:

- load bundled fonts;
- load user-provided font files;
- cache font faces;
- resolve font family/style/weight;
- expose fallback chains;
- provide font metrics;
- support math font lookup.

### Glyph cache

Key glyphs by:

```text
font face id
glyph id
style flags
SDF parameters
atlas page
```

Potential cache key:

```c
struct GlyphKey {
    uint32_t font_id;
    uint32_t glyph_id;
    uint16_t px_range;
    uint16_t sdf_mode;
};
```

### Glyph atlas

The atlas should support:

- one or more 2D texture pages;
- incremental glyph insertion;
- dirty rectangle uploads;
- LRU or reset strategy when full;
- separate atlas pages for incompatible SDF parameters if needed.

Initial implementation can use a single atlas texture, for example 2048×2048 or 4096×4096.

Later implementation should support multiple pages and large Unicode workloads.

### Atlas texture format

Possible formats:

- `r8unorm`: grayscale SDF or alpha bitmap;
- `rgba8unorm`: MSDF/MTSDF;
- `rg8unorm` or compressed variants later if justified.

For MSDF, `rgba8unorm` is usually convenient:

- RGB: multi-channel signed distance;
- A: optional true distance / alpha / metadata.

---

## Shader design

The first glyph shader should be simple and backend-portable.

### Vertex shader

Inputs:

- static quad vertex: local corner coordinates;
- per-instance glyph data: position, offset, size, color, UV rect, flags.

Outputs:

- atlas UV;
- color;
- optional distance parameters.

The shader should support:

- screen-space glyphs;
- world/data-space anchors;
- optional rotation;
- correct viewport/DPI handling.

### Fragment shader

For grayscale SDF:

```text
sample distance
compute alpha using smoothstep around 0.5
apply color alpha
```

For MSDF:

```text
sample RGB
use median RGB distance
apply screen-space derivative correction
compute alpha
```

The implementation should avoid hard-coding backend-specific shader languages into the scene API. In v0.4, WGSL should be the canonical scene shader source, with backend compilation handled through DRP/core.

---

## Integration with Scene and DRP

The text system should be a scene-level feature.

### Scene resources

The scene should create/update resources for:

- glyph instance buffers;
- optional index buffers;
- uniform blocks for transform, viewport, DPI, and style;
- glyph atlas textures;
- math bitmap textures, for fallback mode.

Dirty tracking:

- changing text marks the glyph instance buffer dirty;
- missing glyphs mark atlas texture regions dirty;
- changing font size may trigger relayout and possibly additional atlas entries;
- changing position may only update instance buffer;
- changing color may only update instance buffer or uniform, depending on representation.

### DRP resources

The DRP stream should contain normal commands:

- create/write buffers;
- create/write textures;
- create texture views;
- create sampler;
- create shader modules;
- create bind groups and pipelines;
- draw indexed or draw instanced.

No text-specific GPU API should be required at the DRP level.

---

## Build and packaging strategy

### Build options

Suggested CMake options:

```cmake
DVZ_USE_TEXT=ON
DVZ_USE_FREETYPE=ON
DVZ_USE_HARFBUZZ=ON
DVZ_USE_MSDFGEN=ON
DVZ_USE_MICROTEX=ON
DVZ_BUNDLE_FONTS=ON
```

For minimal/headless builds, text/math may be optional.

For Python wheels and standard binary distributions, text/math should be enabled by default if licensing and build complexity are acceptable.

### Vendoring policy

Dependencies may be handled by one of:

- vendored source subtree;
- Git submodule;
- CMake FetchContent;
- package-manager integration for developers;
- statically linked third-party libraries in binary wheels.

For reproducible wheels and user convenience, avoid relying on system packages at runtime.

### Runtime asset discovery

Datoviz should have a reliable asset lookup mechanism:

```text
1. user-provided font path
2. application-provided asset directory
3. Datoviz bundled font directory
4. optional system font fallback, only if explicitly enabled
```

Basic text/math must work even when step 4 is unavailable.

---

## Public API sketch

The exact API is not fixed. This section only defines likely semantics.

### Text visual

Python-style sketch:

```python
text = panel.text(
    "Voltage (mV)",
    pos=(20, 20),
    coord="screen",
    font="default",
    size=14,
    color=(1, 1, 1, 1),
    anchor="top-left",
)

text.set_string("Updated label")
text.set_color((1, 0.8, 0.2, 1))
```

C-style sketch:

```c
DvzVisual* text = dvz_text(panel);
dvz_text_string(text, "Voltage (mV)");
dvz_text_font(text, "default");
dvz_text_size(text, 14.0f);
dvz_text_color(text, color);
dvz_text_position(text, pos);
dvz_text_anchor(text, DVZ_ANCHOR_TOP_LEFT);
dvz_text_coord(text, DVZ_COORD_SCREEN);
```

### Glyph visual

For advanced users and internal math layout:

```c
DvzVisual* glyphs = dvz_glyph_visual(panel);
dvz_glyph_instances(glyphs, instances, count);
dvz_glyph_font(glyphs, font);
dvz_glyph_atlas(glyphs, atlas);
```

### Math visual

Python-style sketch:

```python
eq = panel.math(
    r"\int_0^\infty e^{-x^2}\,dx = \frac{\sqrt\pi}{2}",
    pos=(0.5, 0.9),
    coord="panel",
    size=18,
    color=(1, 1, 1, 1),
)
```

C-style sketch:

```c
DvzVisual* eq = dvz_math(panel);
dvz_math_latex(eq, "\\int_0^\\infty e^{-x^2}\\,dx = \\frac{\\sqrt\\pi}{2}");
dvz_math_size(eq, 18.0f);
dvz_math_position(eq, pos);
dvz_math_coord(eq, DVZ_COORD_PANEL);
```

### Explicit rendering backend selection

Optional advanced control:

```c
dvz_math_backend(eq, DVZ_MATH_BACKEND_AUTO);
dvz_math_backend(eq, DVZ_MATH_BACKEND_BITMAP);
dvz_math_backend(eq, DVZ_MATH_BACKEND_GLYPHS);
```

Default should be `AUTO`.

---

## Implementation phases

## Phase 1 — Minimal glyph/text visual

Goal: render normal text with bundled fonts.

Tasks:

- integrate FreeType;
- bundle at least one default font;
- load glyph metrics;
- generate raster alpha or SDF atlas entries;
- create glyph atlas texture;
- create glyph instance buffer;
- implement glyph quad shader;
- implement screen-space text rendering;
- support ASCII and basic Unicode where shaping is not required;
- expose minimal Python/C text API.

Acceptance criteria:

- render labels and titles without system fonts;
- render thousands of glyphs interactively;
- support color, size, anchor, and position;
- no external font installation required.

---

## Phase 2 — MSDF/MTSDF and shaping

Goal: robust high-quality text.

Tasks:

- integrate MSDF/MTSDF generation;
- switch atlas format to MSDF/MTSDF where available;
- integrate HarfBuzz;
- support kerning, ligatures, glyph clusters;
- support fallback fonts;
- add multiline layout;
- improve DPI scaling.

Acceptance criteria:

- high-quality glyph rendering across a useful range of sizes;
- correct kerning and shaping for common Latin text;
- robust fallback for missing glyphs;
- no visible atlas artifacts in standard labels.

---

## Phase 3 — MicroTeX bitmap math fallback

Goal: math equations work out of the box.

Tasks:

- vendor/build MicroTeX or equivalent;
- bundle required math fonts/assets;
- expose `MathVisual` API;
- render LaTeX math strings to CPU RGBA/alpha bitmaps;
- upload equation bitmap as texture;
- draw with image/textured-quad visual;
- support color, background transparency, size, DPI, and anchor;
- cache equation bitmaps by expression/style/DPI.

Acceptance criteria:

- render common equations such as:

```latex
\alpha + \beta = \gamma
\frac{a}{b}
\sqrt{x^2 + y^2}
\int_0^\infty e^{-x^2}\,dx
\nabla \cdot \mathbf{E} = \rho / \varepsilon_0
```

- no LaTeX distribution installed;
- works in Python wheels;
- equations can be used as axis labels, titles, and annotations.

---

## Phase 4 — Math glyph-box backend

Goal: render equations through the glyph atlas instead of bitmap textures.

Tasks:

- extract structured layout from MicroTeX;
- convert boxes to glyph instances;
- map math symbols to bundled math font glyphs;
- handle baselines, advances, superscripts, subscripts, fractions;
- support per-glyph color/style where feasible;
- reuse `GlyphVisual` pipeline;
- keep bitmap fallback for unsupported constructs.

Acceptance criteria:

- common equations render sharply at arbitrary zoom;
- equation rendering uses atlas glyphs when possible;
- bitmap fallback remains available and automatic;
- no public API break from Phase 3.

---

## Phase 5 — Advanced math and vector paths

Goal: improve quality and completeness.

Tasks:

- support OpenType MATH table data where necessary;
- improve stretchy delimiters;
- improve radicals, accents, large operators;
- add optional vector path extraction;
- allow SVG/path visual fallback for complex symbols;
- explore GPU vector rendering or CPU tessellation for large text.

Acceptance criteria:

- high-quality rendering for most scientific math annotations;
- clear fallback behavior for unsupported TeX constructs;
- acceptable performance for dynamic annotations.

---

## Performance considerations

### Text rendering

Text should be rendered with batched instancing whenever possible.

Preferred draw model:

```text
one atlas texture
one glyph pipeline
one or few instance buffers
one draw call per text batch / font atlas page / render stage
```

Avoid one draw call per string.

### Atlas updates

Atlas updates should be incremental:

- upload only newly generated glyph rectangles;
- avoid recreating the whole texture each frame;
- prewarm common ASCII glyphs for default fonts;
- cache shaped/layout results for static text.

### Math rendering

Bitmap math fallback can be expensive if rerendered every frame. Cache by:

```text
expression
font/style
size
DPI scale
foreground color mode if baked
backend mode
```

Prefer not to bake color into the bitmap when possible; use alpha mask + shader color if MicroTeX output permits it.

---

## Testing plan

### Unit tests

- font loading from bundled path;
- missing font fallback;
- glyph metrics extraction;
- atlas packing;
- atlas dirty rectangle updates;
- text layout anchors;
- multiline baseline layout;
- UTF-8 decoding;
- HarfBuzz shaping smoke tests;
- MicroTeX parse/render smoke tests.

### Rendering tests

Create deterministic screenshot tests for:

- ASCII text;
- Unicode symbols;
- monospace text;
- rotated text;
- multiline text;
- screen-space labels;
- world-space labels;
- math equations;
- high-DPI scaling;
- transparent background math texture;
- stress test with thousands of labels.

### Example gallery candidates

1. **Text Labels Demo**
   - many labels attached to scatter points;
   - pan/zoom while labels remain readable.

2. **Axis Math Labels Demo**
   - plot with LaTeX axis labels, title, legend.

3. **Formula Overlay Demo**
   - several equations in screen-space annotations.

4. **3D Text Annotation Demo**
   - labels in 3D scene with pixel-size glyphs anchored to world positions.

5. **Font Browser Demo**
   - compare bundled sans, mono, and math fonts.

---

## Risks and open questions

### MicroTeX output model

The most important technical uncertainty is whether the selected MicroTeX implementation exposes enough structured layout information for the glyph-box backend.

If it only provides bitmap output, Phase 3 is still useful, but Phase 4 may require deeper integration or a different math layout engine.

### Math font support

High-quality math rendering requires more than ordinary glyph metrics. Stretchy delimiters, radicals, accents, and large operators may require OpenType MATH table support or engine-specific data.

A minimal implementation can support common formulas first, with documented limitations.

### Licensing

All bundled dependencies and fonts must be license-reviewed.

The final distribution should clearly list:

- third-party libraries;
- bundled fonts;
- font licenses;
- attribution requirements.

### Binary size

Bundling fonts and MicroTeX will increase binary/wheel size. Keep the default font bundle minimal, and optionally allow extended font packs.

### Backend parity

The shader and resource model must work in Vulkan and WebGPU-style backends. Keep the shader portable and avoid Vulkan-only assumptions in the scene layer.

---

## Recommended initial implementation target

The first useful milestone should be:

```text
FreeType + bundled font
basic glyph atlas
simple SDF or alpha glyph shader
screen-space TextVisual
MicroTeX bitmap MathVisual fallback
no external LaTeX dependency
```

This provides immediate value for:

- axis labels;
- annotations;
- legends;
- plot titles;
- mathematical formulas;
- documentation examples.

Then improve quality and generality with MSDF, HarfBuzz, and glyph-box math rendering.

---

## Final design summary

Datoviz should not embed a full LaTeX distribution. Instead, it should embed a lightweight math layout/rendering engine and connect it to a general-purpose GPU glyph system.

Recommended architecture:

```text
TextVisual
    FreeType/HarfBuzz layout
    MSDF/MTSDF glyph atlas
    instanced glyph rendering

MathVisual
    MicroTeX parser/layout
    Phase 1: bitmap texture fallback
    Phase 2: glyph boxes into TextVisual

Scene layer
    owns layout, font cache, atlas cache, dirty tracking

DRP/core layer
    only receives ordinary buffers, textures, samplers, shaders, pipelines, and draw calls
```

This keeps the design lightweight, portable, compatible with the v0.4 scene architecture, and usable out of the box without any external LaTeX installation.
