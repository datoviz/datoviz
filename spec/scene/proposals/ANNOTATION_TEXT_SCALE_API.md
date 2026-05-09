> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-09`
> - **Purpose:** define the intended first scene-facing API shape for text visuals, shared scales,
>   colorbars, measurement annotations, and pinned semantic labels.

# Text, Scale, and Annotation API

This note turns the current text, scale, colorbar, and annotation design decisions into one focused
scene-facing API shape.


## Objective

Define a coherent first API for:

1. text visuals,
2. shared scales,
3. panel colorbars,
4. measurement annotations,
5. pinned semantic labels and readout annotations.


## Existing Grounding In Scene Proposals

This API note consolidates:

1. [spec/scene/proposals/TEXT_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/proposals/TEXT_DESIGN.md)
2. [spec/scene/proposals/COLORBAR_COLORMAP_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/proposals/COLORBAR_COLORMAP_DESIGN.md)
3. [spec/scene/proposals/ANNOTATION_MEASUREMENT_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/proposals/ANNOTATION_MEASUREMENT_DESIGN.md)
4. [spec/scene/proposals/AXES_DOMAIN_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/proposals/AXES_DOMAIN_DESIGN.md)
5. [spec/scene/proposals/PROBE_READOUT_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/proposals/PROBE_READOUT_DESIGN.md)


## Core Recommendation

Keep these APIs layered:

1. semantic objects at scene or panel level,
2. retained content/style objects,
3. derived runtime resources underneath.

Do not let atlas pages, palette textures, or guide geometry become the public API surface.


## Main Scene Objects

Recommended first-class scene-facing concepts:

1. text visual
2. font resource
3. scale object
4. colormap object
5. colorbar object
6. annotation object
7. optional pinned annotation/readout object


## Text Visual Surface

Text should be a visual family with high-level content and placement setters.

Conceptual API shape:

```text
DvzVisual* text = dvz_text(panel, flags);
dvz_text_set_string(text, "CA1");
dvz_text_set_style(text, &style);
dvz_text_set_placement(text, &placement);
dvz_text_set_color(text, rgba);
dvz_text_set_glyph_colors(text, colors, count);
```

Recommended first-slice text concepts:

1. string or text-run content
2. text style object
3. placement object
4. screen-space versus world-space placement mode
5. run-level color with optional per-glyph color override


## Font And Resource Surface

Font and atlas resources should stay scene-owned.

Conceptual API shape:

```text
DvzFontResource* font = dvz_font(scene, path_or_bytes, flags);
dvz_text_set_font(text, font);
```

Public rule:

1. callers bind fonts and semantic text content,
2. callers do not manage atlas pages or glyph UVs directly by default.


## Text Style Surface

Minimal style should be explicit but small.

Conceptual style fields:

1. font reference
2. size
3. color
4. bold flag
5. italic flag
6. underline flag
7. alignment/anchor

Recommended first-slice rule:

1. keep style flags semantic,
2. do not expose one giant typography engine in the first API.


## Text Placement Surface

Placement should be explicit and not hidden in one transform setter.

Conceptual API shape:

```text
dvz_text_set_screen_position(text, panel_xy)
dvz_text_set_world_position(text, xyz)
dvz_text_set_orientation_mode(text, mode)
dvz_text_set_scale_mode(text, mode)
```

Recommended placement modes:

1. screen-space
2. world-space
3. hybrid/world-anchor plus screen-facing layout later


## Scale Surface

Scales should be scene-owned semantic objects.

Conceptual API shape:

```text
DvzScale* scale = dvz_scale(scene, DVZ_SCALE_CONTINUOUS);
dvz_scale_set_domain(scale, min, max);
dvz_scale_set_view_range(scale, vmin, vmax);
dvz_scale_set_units(scale, "mm");
dvz_scale_set_colormap(scale, cmap);
```

Recommended scale semantics:

1. semantic domain
2. visible view/window
3. formatting metadata
4. optional referenced colormap


## Colormap Surface

Colormaps should be explicit semantic objects that scales can reference.

Conceptual API shape:

```text
DvzColormap* cmap = dvz_colormap(scene, DVZ_COLORMAP_CONTINUOUS);
dvz_colormap_set_builtin(cmap, DVZ_CMAP_VIRIDIS);
dvz_colormap_set_stops(cmap, stops, count);
dvz_colormap_set_center(cmap, center);
dvz_scale_set_colormap(scale, cmap);
```

Recommended first-slice semantics:

1. builtin named perceptual maps,
2. custom color-stop ramps,
3. diverging maps with explicit center,
4. categorical palettes later as a separate semantic case when needed.

Recommended ownership:

1. colormap is a scene-owned semantic object,
2. one scale references one active colormap at a time,
3. colorbars explain the scale-colormap pair without owning either object.


## Colorbar Surface

Colorbars should be panel-owned semantic objects bound to scales.

Conceptual API shape:

```text
DvzColorbar* cb = dvz_colorbar(panel, scale, flags);
dvz_colorbar_set_orientation(cb, mode);
dvz_colorbar_set_title(cb, "Intensity");
dvz_colorbar_set_anchor(cb, anchor);
dvz_colorbar_set_format(cb, format_override);
```

Recommended rule:

1. colorbars explain scales,
2. they do not become the owner of the scale,
3. they inherit colormap appearance from the referenced scale unless an explicit explanatory
   override is introduced later.


## Annotation Surface

Measurement annotations should be explicit semantic objects.

Conceptual API shape:

```text
DvzAnnotation* ann = dvz_annotation(panel, kind, flags);
dvz_annotation_set_style(ann, &style);
```

Recommended first-slice kinds:

1. scale bar
2. dimension
3. bounding box helper
4. label/callout
5. pinned readout label


## Measurement API Surface

Measurement annotations should expose semantic setters, not only geometry setters.

Conceptual API shape:

```text
dvz_scalebar(panel, flags)
dvz_scalebar_set_units(sb, units)
dvz_scalebar_set_anchor(sb, anchor)

dvz_dimension(panel, flags)
dvz_dimension_set_points(dim, p0, p1, space)
dvz_dimension_set_units(dim, units)
```

Recommended rule:

1. callers provide anchors, units, and intent,
2. the annotation system derives label text and guide geometry.


## Formatting Policy Surface

Formatting should reuse shared panel/domain/scale machinery by default.

Conceptual API shape:

```text
dvz_panel_set_format_defaults(panel, &fmt)
dvz_scale_set_format(scale, &fmt_override)
dvz_annotation_set_format(ann, &fmt_override)
```

Recommended rule:

1. panel/domain/scale defaults come first,
2. per-object local overrides layer on top,
3. do not fork separate bespoke formatters for text, scales, and measurements.


## Pinned Readout / Label Surface

Pinned readouts and semantic labels should fit the same annotation family.

Conceptual API shape:

```text
DvzAnnotation* label = dvz_pinned_label(panel, &probe_result, flags);
dvz_annotation_set_format(label, ...);
dvz_annotation_set_link_channel(label, channel);
```

Recommended first-slice behavior:

1. multiple pinned labels/readouts allowed
2. retained scene-owned objects
3. passive by default
4. may live-update with shared formatting/domain changes


## Interaction Relationship

These APIs should integrate with interaction without merging into it.

Recommended rule:

1. text/annotation objects may be selected or linked,
2. pinned labels may be created from probe results,
3. scales and colorbars may respond to external UI edits,
4. none of these APIs should own picking or controller logic directly.


## Example Conceptual Flow

```text
scale = dvz_scale(scene, DVZ_SCALE_CONTINUOUS)
dvz_scale_set_domain(scale, 0, 4095)
dvz_scale_set_view_range(scale, 200, 1200)
dvz_scale_set_units(scale, "HU")

cmap = dvz_colormap(scene, DVZ_COLORMAP_CONTINUOUS)
dvz_colormap_set_builtin(cmap, DVZ_CMAP_VIRIDIS)
dvz_scale_set_colormap(scale, cmap)

image = dvz_image(panel, flags)
dvz_visual_set_scale(image, scale)

cb = dvz_colorbar(panel, scale, flags)
dvz_colorbar_set_title(cb, "Intensity")

text = dvz_text(panel, flags)
dvz_text_set_string(text, "CA1")
dvz_text_set_world_position(text, xyz)

sb = dvz_scalebar(panel, flags)
dim = dvz_dimension(panel, flags)
```


## Immediate Scope Recommendation

The narrowest useful first API slice is:

1. scene-owned font resource
2. text visual with string/style/placement setters
3. scene-owned continuous scale
4. scene-owned colormap bound to that scale
5. panel-owned colorbar bound to scale
6. scale bar and dimension annotations
7. pinned readout labels as retained annotation objects


## Explicit Non-Goals For The First Slice

1. exposing atlas UVs or palette textures as the primary public interface
2. full typography or figure-layout systems
3. one monolithic annotation object that hides all semantic kinds
4. backend-native text or texture handles in the public scene layer
