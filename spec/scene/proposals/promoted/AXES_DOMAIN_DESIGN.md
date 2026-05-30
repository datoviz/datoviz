> **Execution Status**
> - **Status:** `PARTIALLY SUPERSEDED PROPOSAL`
> - **Updated on:** `2026-05-25`
> - **Purpose:** define the intended v0.4 panel-domain, axes, and unit-semantics model for 2D
>   scientific views and the first 3D orientation aids.

# Axes and Domain Design

This note narrows the larger scene axes discussion into the contract that informed the landed
linear X/Y axis and scale-bar first slices. Installed headers and `spec/scene/semantics/AXES.md`
are authoritative for APIs that now exist.


## Objective

Define how panels, domains, axes, and unit-aware explanatory objects should fit together so that:

1. 2D scientific panels have real domain semantics,
2. panzoom and axis regeneration stay coherent,
3. annotations and scale bars can share unit logic,
4. early 3D views can expose useful orientation aids without pretending they are the same as 2D axes.


## Existing Grounding In The Repo

Relevant context already exists in:

1. broad axis semantics:
   [spec/scene/semantics/AXES.md](../../semantics/AXES.md)
2. scale and unit-adjacent mappings:
   [spec/scene/semantics/SCALES.md](../../semantics/SCALES.md)
3. transform/controller direction:
   [TRANSFORM_CONTROLLER_DESIGN.md](TRANSFORM_CONTROLLER_DESIGN.md)
4. measurement overlays:
   [ANNOTATION_MEASUREMENT_DESIGN.md](ANNOTATION_MEASUREMENT_DESIGN.md)

This note records the active v0.4 recommendation at the level most likely to affect implementation
soon.


## Core Recommendation

Panels should own domain semantics, and axes should be panel-aware semantic objects derived from
those domains.

Recommended split:

1. panel owns domain/view semantics,
2. axes derive from panel domain plus panel navigation state,
3. visuals render inside that domain contract,
4. measurement overlays can reuse unit formatting and visible-scale information from the same panel
   domain state.

Do not make axes mere decorative visuals.


## Why Panel-Owned Domains

Panels are the right place for domain semantics because they already own:

1. viewport state,
2. navigation/controller state,
3. camera/projection context,
4. visual attachment.

That makes them the right owner for:

1. visible 2D data ranges,
2. axis configuration,
3. unit formatting defaults,
4. linked navigation behavior.


## 2D Domain Model

The first explicit domain model should target 2D panels.

Recommended baseline:

1. one X domain
2. one Y domain
3. each domain carries numeric range and optional unit metadata
4. panzoom acts on the visible domain rather than on raw arbitrary transforms alone

This gives a stable semantic home for:

1. ticks,
2. labels,
3. scale bars,
4. dimension formatting,
5. linked panels.


## Domain Versus Visual Geometry

The panel domain model should stay distinct from geometry coordinates.

Recommended rule:

1. visuals may still use normalized or already-derived visual-ready geometry internally,
2. panel domains record the semantic coordinate system visible to the user,
3. axes and measurement formatting come from the semantic domain, not from accidental vertex values.

This is the same separation that makes axes special in the broader spec.


## Axis Ownership

Axes should be panel-owned semantic objects, not mesh-owned and not data-resource-owned.

Recommended behavior:

1. panel can expose X and Y axes independently,
2. each axis uses the panel’s domain plus current visible extent,
3. axis objects derive labels, tick marks, and optional grid lines,
4. rendered resources remain derived outputs rather than authored user geometry.


## Tick Generation Policy

Axis tick generation should follow the same nice-value philosophy as scale bars and dimensions.

Recommended baseline:

1. nice-step ladder based on `1 / 2 / 5 * 10^n`,
2. tick density constrained by screen readability,
3. label formatting driven by unit-aware value formatting,
4. regeneration only when the current tick layout is no longer suitable.

Do not regenerate on every pan/zoom event if the existing covered domain remains acceptable.


## Unit Ownership

Axes and measurements should share unit-formatting helpers.

Recommended rule:

1. domains may carry default unit metadata,
2. axes format values through shared unit helpers,
3. scale bars and dimensions use the same unit conversion and nice-step logic,
4. scales/colorbars may reuse compatible unit metadata where relevant.

This is the most important integration point between axes and the measurement system.


## Linked Panels

Linked-panel behavior should be domain-driven.

Recommended behavior:

1. panels may share or link X and/or Y domains,
2. navigation updates propagate through linked domain state,
3. each panel still derives its own axis resources and overlay layout locally.

This keeps domain semantics shared without forcing all derived geometry and labels to be identical.


## 3D Axes Versus 3D Orientation Aids

Do not force full 3D axes into the same model as 2D scientific axes in the first pass.

Recommended split:

1. 2D axes are domain-semantic objects,
2. early 3D helpers are orientation/measurement aids,
3. full 3D scientific axis semantics can come later if a concrete need appears.

For the active roadmap, the first useful 3D aids are:

1. object-space or world-space orientation triad,
2. world-space bounding box overlay,
3. dimension annotations with units,
4. adaptive scale/reference indicator where meaningful.


## 3D Scale Reference

The scale-bar requirement interacts with 3D views too.

Recommended policy:

1. a 3D panel may expose a screen-space reference scale derived from projected world scale,
2. this remains a measurement overlay, not a 3D axis,
3. the chosen reference length should follow the same nice-step and unit-formatting rules as the
   2D scale bar.

This keeps the concept coherent across 2D and 3D without pretending there is one unified axis type.


## Domain Formatting Policy

The domain layer should own formatting policy defaults.

Recommended fields per domain:

1. numeric range
2. optional unit label/system
3. formatting precision policy
4. scale type such as linear or log

This is enough for:

1. axis labels,
2. cursor probes later,
3. scale bars,
4. dimension readouts.


## Log And Nonlinear Domains

The domain model should allow nonlinear semantics, even if the first implementation starts narrow.

Recommended direction:

1. linear domains first,
2. reserve domain scale type for later log or other nonlinear mappings,
3. keep tick generation owned by domain semantics rather than hardcoded to linear assumptions
   everywhere.

This prevents a needless redesign later.


## Relationship To Controllers

For 2D panels, controller behavior should be phrased in domain terms where possible.

Recommended behavior:

1. panzoom updates visible domain state,
2. axis regeneration logic observes domain and viewport changes,
3. measurement overlays like the scale bar read the same visible-scale information,
4. world-space 3D helpers remain governed by the transform/controller rules from the transform note.


## Relationship To Picking

Axes and related overlays may become pickable, but that is not the main architectural issue.

Recommended first rule:

1. axis contributions may be pickable at the axis-object level later,
2. domain-aware cursor probes should resolve semantic values from panel domain state,
3. measurement overlays and axes should not invent a separate coordinate system for interaction.


## Relationship To Text

Axes are a major consumer of the text system, but they should not define the text architecture.

Recommended split:

1. axes decide tick values and label strings,
2. text system lays out and renders those strings,
3. annotation/measurement system handles scale-bar and dimension label semantics separately while
   reusing the same text backend.


## Initial Public API Direction

The landed first-slice names differ from the earlier sketch, but the conceptual API supports:

1. configuring panel X/Y domains,
2. enabling/configuring X/Y axes per panel,
3. linking panel domains,
4. reading shared visible-scale information for measurement overlays.

Current surface concepts:

1. `dvz_panel_set_domain(panel, dim, min, max)`
2. `dvz_panel_visible_domain(panel, dim, &min, &max)`
3. `dvz_panel_axis(panel, dim)`
4. `dvz_axis_set_visible(axis, visible)`
5. `dvz_axis_set_grid(axis, visible)`
6. `dvz_axis_set_label(axis, label)`
7. linked-domain behavior remains follow-up work

The implementation can remain flexible as long as the semantics stay panel-owned.


## Immediate Scope Recommendation

The narrowest useful first implementation slice is:

1. panel-owned 2D numeric X/Y domains,
2. unit-aware X/Y axes with retained regeneration policy,
3. shared nice-step/unit formatting used by adaptive scale bars,
4. basic 3D orientation helper support treated as annotations, not full 3D axes.


## Explicit Non-Goals For The First Slice

1. fully general 3D scientific axis systems,
2. every categorical/time/date axis mode immediately,
3. a complete legend/colorbar system in the same first pass,
4. coupling axis semantics directly to one visual family’s geometry layout.
