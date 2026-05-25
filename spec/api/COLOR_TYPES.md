# Color Types

Status: normative v0.4 API design target.

This document defines the target public color model for Datoviz v0.4. The current array-alias
`DvzColor` should be treated as an implementation artifact to replace before API freeze.


## Goals

1. Make color semantics explicit at the type and function boundary.
2. Keep the common visual attribute layout compact: 4 bytes per RGBA8 color.
3. Keep style APIs pleasant in C and easy to bind in other languages.
4. Avoid manual float-to-byte conversion in examples, tests, and user code.
5. Distinguish display/UI colors from linear-light math and shader/material factors.
6. Avoid packed integer color storage as the primary API because byte order is easy to misuse.


## Canonical Color Types

The public byte color type is a real struct:

```c
typedef struct DvzColor
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} DvzColor;
```

`DvzColor` is the canonical RGBA8 user color:

1. RGB channels are display-encoded color channels suitable for UI, palettes, categorical colors,
   text, axes, and ordinary visual styling.
2. Alpha is straight, not premultiplied.
3. Channel order is always `r, g, b, a` in memory.
4. Zero initialization means transparent black: `{0, 0, 0, 0}`.
5. `sizeof(DvzColor)` must be 4. The implementation must assert this at build time.

`DvzColor` remains the default bulk visual color attribute type. GPU upload code should continue to
use RGBA8 unorm vertex attributes or texture data where that is the intended compact format.


## Float Color Type

The public float color type is separate:

```c
typedef struct DvzColorf
{
    float r;
    float g;
    float b;
    float a;
} DvzColorf;
```

`DvzColorf` is for linear-light math, shader uniforms, material factors, clear colors, interpolation
that should not be done in byte space, HDR-capable paths, and internal conversions that need more
precision than RGBA8.

Rules:

1. `DvzColorf` uses straight alpha.
2. RGB values may exceed `[0, 1]` where an API explicitly supports HDR or intensity factors.
3. Alpha should normally be clamped by consuming APIs to `[0, 1]`.
4. An API that wants display-space normalized floats must say so in its name or documentation. It
   should not silently reuse `DvzColorf` as if it were both display-encoded and linear.


## Constructors And Conversion Helpers

The public API should provide small inline constructors and conversion helpers in the same public
header that defines the color types.

Required helpers:

```c
static inline uint8_t dvz_color_u8(float value);
static inline DvzColor dvz_color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
static inline DvzColor dvz_color_rgb(uint8_t r, uint8_t g, uint8_t b);
static inline DvzColor dvz_color_from_unit(float r, float g, float b, float a);
static inline DvzColorf dvz_colorf(float r, float g, float b, float a);
static inline DvzColorf dvz_color_to_linear(DvzColor color);
static inline DvzColor dvz_color_from_linear(DvzColorf color);
```

`dvz_color_u8()` defines the scalar policy for normalized float to byte conversion:

1. Non-finite values convert to `0`.
2. Values `<= 0` convert to `0`.
3. Values `>= 1` convert to `255`.
4. Intermediate values round to nearest byte using `value * 255 + 0.5`.

`dvz_color_from_unit()` takes display-space normalized floats and returns `DvzColor`. It exists for
GUI pickers, examples, and simple user-authored color ramps. It is not a replacement for
linear-light interpolation where color accuracy matters.

The linear conversion helpers should use the standard sRGB transfer function for RGB. Alpha is copied
as a normalized straight-alpha value.

Hex helpers are allowed, but they must make byte order explicit in the name:

```c
static inline DvzColor dvz_color_hex_rgb(uint32_t rgb);
static inline DvzColor dvz_color_hex_rgba(uint32_t rgba);
```

`dvz_color_hex_rgba(0xRRGGBBAA)` means the high byte is red and the low byte is alpha. The packed
integer is only a constructor input; it is not the storage model.


## API Signature Rules

Style and scalar appearance APIs should pass `DvzColor` by value:

```c
int dvz_polygon_fill_color(DvzPolygon* polygon, DvzColor color);
int dvz_text_color(DvzText* text, DvzColor color);
int dvz_axis_grid_color(DvzAxis* axis, DvzColor color);
```

These APIs must not check `color == NULL`, because color is a value.

Bulk data APIs should use pointer-plus-count:

```c
int dvz_visual_set_colors(DvzVisual* visual, const DvzColor* colors, uint32_t count);
int dvz_colormap_sample_many(
    const DvzColormap* colormap, const double* values, DvzColor* out, uint32_t count);
```

Output APIs should use an output pointer:

```c
bool dvz_colormap_sample(const DvzColormap* colormap, double value, DvzColor* out);
```

`DvzColorf` should be used by value for scalar material and clear-color APIs when the color is a
coherent record:

```c
int dvz_material_base_color(DvzMaterial* material, DvzColorf color);
int dvz_panel_clear_color(DvzPanel* panel, DvzColorf color);
```

Avoid APIs that take `uint8_t color[4]`, `float color[4]`, or `uint32_t rgba` as the primary public
shape unless the function is explicitly about raw image bytes or binary interchange.


## Visual Attributes And Images

Visual families that currently declare `"color"` as RGBA8 should use `sizeof(DvzColor)` as the item
size. Their public examples should allocate and fill `DvzColor colors[count]`.

Raw image APIs may continue to accept `const uint8_t* rgba` when the caller is passing image bytes
from external codecs or memory-mapped files. Higher-level Datoviz-owned image helpers may use
`const DvzColor* pixels` when they specifically require tightly packed RGBA8 pixels.

Do not use `DvzColor` for scalar fields, color-mapped data, depth, masks, or arbitrary four-channel
textures. Those should keep explicit data formats.


## Color Spaces And Alpha

Public APIs must not leave color space and alpha representation implicit when the distinction affects
rendering or data interchange.

Defaults:

1. `DvzColor` is display-encoded RGBA8 with straight alpha.
2. `DvzColorf` is linear-light RGBA float with straight alpha unless a function explicitly documents
   another color space.
3. Premultiplied alpha is an internal or explicitly named interchange format. Public style APIs
   should not accept premultiplied colors without naming that policy.

If a future API needs explicit color-space negotiation, use an enum in descriptors rather than a
global switch:

```c
typedef enum DvzColorSpace
{
    DVZ_COLOR_SPACE_SRGB = 0,
    DVZ_COLOR_SPACE_LINEAR = 1,
} DvzColorSpace;
```

Do not add display-P3, ICC profiles, or full color management until there is an implementation path
and a validation story. The v0.4 target is explicit, consistent sRGB and linear behavior, not a full
CMS.


## Palettes, Colormaps, And Interpolation

Palettes and categorical colors should store `DvzColor`.

Continuous colormaps may expose `DvzColor` samples for convenience and compact upload, but internal
interpolation should be defined explicitly:

1. Perceptual or authored lookup tables may interpolate in table space if documented.
2. Physically meaningful blending and material interpolation should use `DvzColorf` linear values.
3. Colormap sample APIs should provide both byte and float forms when both are useful:

```c
bool dvz_colormap_sample(const DvzColormap* colormap, double value, DvzColor* out);
bool dvz_colormap_samplef(const DvzColormap* colormap, double value, DvzColorf* out);
```


## Examples And Tests

Examples must not define local helpers such as `_u8(float)` or `_alpha_u8(float)` for normalized
float to byte conversion. They should use `dvz_color_u8()` for one channel and
`dvz_color_from_unit()` for full colors.

Tests should cover:

1. `sizeof(DvzColor) == 4`.
2. `offsetof(DvzColor, r/g/b/a) == 0/1/2/3`.
3. `dvz_color_u8()` clamping, rounding, and non-finite behavior.
4. `dvz_color_from_unit()` channel order.
5. `DvzColor` arrays matching the GPU RGBA8 upload layout.
6. sRGB-to-linear and linear-to-sRGB round trips within a documented tolerance.


## Migration Plan

The v0.4 branch can make this change aggressively because external API compatibility is not a
constraint.

1. Replace `#define DvzColor cvec4` with `typedef struct DvzColor { ... } DvzColor`.
2. Add `DvzColorf`, constructors, conversion helpers, and compile-time layout assertions.
3. Update style APIs to pass `DvzColor` by value and remove NULL checks on color values.
4. Update output APIs to use `DvzColor* out`.
5. Update visual attribute definitions to use `sizeof(DvzColor)` and keep RGBA8 unorm formats.
6. Replace `color[0]` access with `color.r` access in scalar code.
7. Keep `DvzColor colors[count]` as the bulk data shape.
8. Update examples to use constructors instead of manual byte conversion.
9. Add focused tests for layout, conversion, and representative visual uploads.
10. Remove compatibility shims before API freeze unless they are needed for internal generated code.


## Non-Goals

1. Do not introduce a full color-management system in this pass.
2. Do not make all colors floats. RGBA8 remains the right compact visual attribute format.
3. Do not use packed `uint32_t` as the canonical public color type.
4. Do not preserve array-decay compatibility with the old `DvzColor` macro alias.
5. Do not overload one type to mean both byte display color and linear float material color.

