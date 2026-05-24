> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-20`
> - **Purpose:** preserve the remaining proposal-stage text decisions after the normative text,
>   rendering-slice, and shaping/atlas contracts absorbed the durable rules.

# Text Design

## Decision Addressed

Datoviz v0.4 treats text as a first-class scene feature, not a backend overlay or a
per-glyph-only visual API.

The proposal-stage question that remains is how far to reserve for equation backends, color fonts,
and future non-atlas renderers while the implementation starts with shared font/atlas resources.


## Short Summary

The chosen architecture splits text into:

1. semantic content, style, placement, and identity;
2. shaping/layout;
3. font and glyph resource ownership;
4. atlas-backed glyph rendering for the first implementation;
5. optional equation or direct-outline backends later.

Simple labels stay high-level. Advanced callers may provide shaped runs or display lists, but public
scene APIs should not expose atlas UVs as the primary text model.


## Chosen Direction

| Topic | Direction |
|---|---|
| Text ownership | Scene text objects own content, style, placement, transform, and semantic identity. |
| Resource ownership | Scene-owned font and glyph-atlas resources are shared; text objects do not own private atlases by default. |
| Font defaults | `DvzFontDefaults` is the shared policy descriptor used by app, GUI, and scene text; runtime objects remain backend-owned (`ImFont*` for GUI, `DvzFont*`/atlases for scene). |
| Placement | Support screen-space and world-space text explicitly. World-space text may billboard or use a fixed text plane. |
| Scale | Keep screen/pixel scale and world-unit scale as explicit modes. |
| Styling | Start with run color, optional per-glyph color, regular/bold/italic, and underline as a derived decoration. |
| Picking | Start with object/string-level identity; glyph-level picking is deferred. |
| DPI | Scene sizes remain logical; runtime resources patch or rebuild when DPI changes. |
| Rendering backend | Atlas-backed glyph rendering is first; direct GPU outline rendering remains a future backend option. |
| Equation backend | Datoviz does not implement full TeX; a frontend/backend may emit glyph runs, rules, boxes, and transforms. |


## Canonical Migration Links

The authoritative rules now live in:

1. [Scene Text](../../semantics/TEXT.md) for text semantics, placement, style, DPI, resource
   boundaries, interaction, and non-goals;
2. [Text Rendering Slice](../../slices/TEXT_RENDERING_SLICE.md) for the first implementation-ready
   rendering packet;
3. [Text Shaping And Atlas](../../implementation/TEXT_SHAPING_ATLAS.md) for shaping, atlas
   resources, cache keys, and DRP2 emission;
4. [Geometry Utilities](../../semantics/GEOMETRY_UTILITIES.md) for shared SDF/MSDF and font-atlas
   utility placement;
5. [Annotations](../../semantics/ANNOTATIONS.md) for annotation consumers of text.

Do not duplicate those rules here when editing nearby text.


## Remaining Unresolved Points

1. Final public C names for font, atlas, text object, shaped-run, and equation-display-list APIs.
2. Whether the first implementation exposes shaped-run ingestion publicly or keeps it internal.
3. Exact color-font and emoji capability model and fallback diagnostics.
4. Whether direct GPU outline rendering is only an internal backend or a selectable capability.
5. Final object-level text picking payload shape and its relationship to annotations and readouts.
6. Broader bundled font policy for predictable scientific and equation output beyond the current shared
   default descriptor.
