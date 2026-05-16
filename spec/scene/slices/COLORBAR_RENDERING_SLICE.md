# Colorbar Rendering Slice

This slice defines the first rendered continuous colorbar path for `DvzColorbar`.

It implements a panel-attached colorbar that explains a scene-owned continuous color scale.


## Scope

Implement visible continuous colorbars bound to `DvzScale`.

The first slice supports:

1. one panel-attached `DvzColorbar`,
2. vertical or horizontal orientation,
3. panel-edge anchoring from `DvzColorbarDesc.anchor`,
4. a continuous ramp derived from the referenced scale and colormap,
5. title text,
6. generated tick labels,
7. unit and precision formatting from scale/colorbar format state,
8. offscreen and GLFW app rendering through scene -> `FramePlan` -> DRP2.


## Non-Goals

Do not implement these in the first colorbar slice:

1. categorical legends,
2. grouped or multi-scale legends,
3. interactive range editing,
4. threshold markers,
5. collision-avoidance layout,
6. exported vector text preservation,
7. volume transfer-function colorbars that combine color and opacity.


## Public API Boundary

Use the installed APIs:

1. `dvz_scale()`,
2. `dvz_scale_set_domain()`,
3. `dvz_scale_set_colormap()`,
4. `dvz_scale_set_format()`,
5. `dvz_colormap()`,
6. `dvz_colormap_set_builtin()`,
7. `dvz_colormap_set_stops()`,
8. `dvz_colorbar()`,
9. `dvz_colorbar_destroy()`,
10. `dvz_colorbar_set_format()`.

If an installed setter is missing for a required field, add the narrow setter rather than encoding
the behavior through flags.


## Retained State

Use existing retained scale, colormap, and colorbar state as source of truth:

1. colorbar panel,
2. colorbar scale reference,
3. orientation,
4. anchor,
5. title,
6. colorbar format override,
7. scale kind,
8. scale domain and unit,
9. scale colormap reference,
10. colormap builtin or stop list.

The colorbar does not own the scale or colormap.


## Derived Layout

The first implementation should use deterministic fixed-size layout:

1. reserve an inner panel-edge band for the colorbar,
2. ramp rectangle has stable pixel thickness,
3. title and tick labels use the text renderer,
4. tick marks use primitive or segment contributions,
5. no collision solver is required.

If a panel is too small, validation or adaptation should produce a diagnostic and skip optional
labels before it changes the semantic scale mapping.


## Tick Policy

The first tick generator should be simple and deterministic:

1. use 5 target ticks by default,
2. use the same `1 / 2 / 5 * 10^n` nice-step ladder as measurement annotations,
3. clamp generated ticks to the scale domain,
4. include endpoints when practical,
5. format labels with the scale format, then colorbar override if present.

Log and categorical scales are out of this slice unless already fully represented in the active
`DvzScale` state and validation path.


## Ramp Generation

The first ramp may be represented as either:

1. a small generated RGBA texture sampled by an image-like quad, or
2. a strip of colored primitive rectangles.

Prefer the generated texture path if it reuses existing image/texture upload and sampling
infrastructure cleanly. The public colorbar semantics must not expose which representation is used.


## Dirty Rules

Colorbar invalidation sources:

1. colorbar orientation, anchor, title, or format changes,
2. scale domain, unit, kind, or format changes,
3. colormap builtin, center, or stops change,
4. panel size or DPI changes,
5. text renderer atlas changes.

Changing only the colorbar anchor should dirty layout, not rebuild the scale or visual data.


## Validation

Validate before planning:

1. colorbar panel and scale belong to the same scene,
2. scale kind is continuous color for this slice,
3. scale domain is finite and `domain_min < domain_max`,
4. colormap exists or a documented default colormap is available,
5. title and generated tick labels are valid text inputs,
6. runtime supports the chosen ramp representation and text rendering.

Categorical scales should produce an explicit "legend required" or "categorical colorbar
unsupported" diagnostic in this slice.


## FramePlan Contribution

A rendered colorbar contributes:

1. one ramp render contribution,
2. tick mark primitive/segment contributions,
3. title and tick label glyph contributions,
4. optional background or border contribution only if required for readability.

These should be panel-local overlay contributions that respect panel viewport/scissor.


## Tests

Add focused scene tests for:

1. colorbar creation emits ramp, tick mark, and text contributions,
2. vertical and horizontal orientations use different deterministic layout,
3. scale domain changes regenerate ticks and ramp metadata,
4. colormap changes regenerate the ramp but not unrelated visual geometry,
5. destroyed colorbars stop emitting,
6. cross-scene scale binding remains rejected,
7. categorical scale colorbar request produces an explicit diagnostic.

Add one app/offscreen smoke that captures an image or volume slice with a visible colorbar.


## Acceptance

This slice is complete when:

1. a retained `DvzColorbar` renders a visible continuous ramp with title and tick labels,
2. the colorbar references, but does not own, the scale and colormap,
3. updates to scale domain and colormap are visible across repeated frames,
4. validation catches unsupported scale kinds and invalid domains,
5. focused tests cover creation, updates, destroy, and cross-scene failures.

