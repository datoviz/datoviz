# Scene Axes

This document defines how axes should work in the future scene layer.

Axes are scene-side semantic objects.

They are not merely decorative line visuals, and they are not backend concepts.


## Purpose

Axes exist to:

1. expose data-space semantics to the user,
2. generate ticks and labels from original data coordinates,
3. stay synchronized with live panel navigation,
4. emit ordinary visual-family contributions for rendering.


## Core Rule

Axes must understand both:

1. original data-space semantics,
2. current panel-local viewing state.

This makes axes different from most ordinary visuals.


## Why Axes Are Special

Most visual families can render from normalized visual-ready data plus panel-local transforms.

Axes cannot rely on that alone because:

1. tick selection is a data-space decision,
2. label formatting is a data-space decision,
3. visible tick density depends on current panel view,
4. axis layout must still follow live panzoom or camera state.

An axis therefore sits across the boundary between:

1. `DataSpace` semantics,
2. `VisualSpace` geometry preparation,
3. panel-local viewing transforms.


## Scene-Level Role

Axes should be modeled as panel-aware semantic scene objects.

They should not be modeled as a primitive visual family in the same sense as:

1. `point`
2. `marker`
3. `path`
4. `image`
5. `mesh`

Instead, an axis should be understood as a scene object that produces derived contributions to those
families where appropriate.


## Main Responsibilities

An axis object should be responsible for:

1. knowing the relevant data-space domain,
2. choosing tick values,
3. formatting labels,
4. mapping tick anchors into visual-ready coordinates,
5. creating derived geometry or text contributions,
6. reacting to panel-local navigation changes.


## Coordinate-Space Role

The axis pipeline should follow the general transform model in `TRANSFORM_PIPELINE.md`.

For axes, the important split is:

1. tick values live in `DataSpace`,
2. tick geometry is built in `VisualSpace`,
3. panel-local transforms move the resulting geometry afterward.


## Axis Pipeline

The expected axis pipeline is:

1. determine the currently visible data-space domain,
2. compute tick values in `DataSpace`,
3. format labels from those data values,
4. map tick anchor positions into `VisualSpace`,
5. build tick marks, axis lines, and labels as derived scene contributions,
6. let panel-local transforms move those contributions live.


## Data-Space Semantics

Axis semantics should remain expressed in original data coordinates.

Examples of data-space decisions:

1. choosing “nice” linear tick values,
2. choosing log-scale tick positions,
3. formatting labels from numeric values,
4. later, formatting dates, times, or categorical values.

Those decisions should not be made from already-normalized `[-1, 1]` coordinates alone.


## Tick Representation

Each tick should conceptually carry at least two representations:

1. its original semantic value in `DataSpace`,
2. its derived anchor position in `VisualSpace`.

This split is essential because:

1. the label text comes from the semantic value,
2. the rendered tick mark and label placement come from the visual-ready position.


## Relationship To Panel Navigation

Axes must respond to live panel-local navigation.

For 2D panels:

1. panzoom changes the visible data range,
2. the visible tick set may need to be recomputed,
3. the resulting tick geometry must still follow the active panel transform.

For 3D panels:

1. camera motion changes view framing,
2. axis visibility and placement may need panel-aware updates,
3. rendered axis contributions must still move with the panel view.


## Axis Regeneration Policy

Axes should not be forced to recompute and reupload ticks on every frame during continuous pan or
zoom.

The preferred policy is to separate:

1. cheap live movement through the current panel transform,
2. occasional semantic tick regeneration when the current axis layout is no longer good enough.


## Covered Domain Versus Visible Domain

An axis should be free to maintain a tick layout over a data-space domain that is larger than the
currently visible domain.

Conceptually:

1. `visible_data_domain` is what the panel currently shows,
2. `covered_data_domain` is the larger domain for which ticks and labels are already prepared.

If the visible domain remains comfortably inside the covered domain, the scene should usually:

1. keep the existing axis-derived resources,
2. let panel transforms move them live,
3. avoid tick recomputation and reupload.


## Regeneration Triggers

Tick regeneration should usually happen only when one or more layout invariants stop being satisfied.

Typical triggers:

1. the visible domain approaches or exits the covered domain,
2. zoom changes enough that major tick spacing becomes too dense or too sparse on screen,
3. panel size changes enough that label readability or density changes materially,
4. scale policy or formatting policy changes.

This means the trigger should not be:

1. “the camera moved” or
2. “the panzoom state changed”

by themselves.

Instead, the trigger should be:

1. “the current semantic tick layout is no longer acceptable for the current view”.


## Coverage Margin

The preferred implementation strategy is to generate ticks beyond the visible domain.

That margin allows:

1. smooth live panning without immediate semantic regeneration,
2. fewer uploads during interaction,
3. a clear threshold before regeneration becomes necessary.

The exact margin policy is an implementation choice, but the scene spec should allow:

1. offscreen tick coverage on both sides of the visible domain,
2. regeneration only when the visible domain gets too close to the coverage boundary.


## Important Consequence

Panzoom or camera changes may invalidate axis layout.

This is stronger than for many ordinary visuals.

For many visual families:

1. navigation changes only panel-local transforms,
2. normalized visual resources can remain unchanged.

For axes:

1. navigation may change which ticks should exist,
2. navigation may change label density or placement,
3. navigation may therefore require semantic regeneration of axis contributions.

But importantly:

1. this does not imply regeneration on every frame,
2. axes should be allowed to retain and reuse a previously uploaded semantic layout while the current
   view stays within acceptable bounds.


## Axis Components

An axis object should be free to emit multiple contribution types.

The most likely derived contributions are:

1. `segment` for axis lines and tick marks,
2. `glyph` for tick labels and axis labels,
3. `path` for grid lines when modeled separately,
4. `image` for closely related colorbar-like annotation when needed.

This does not make axes equivalent to those families.
It means axes should be implemented in terms of them.


## Composite Nature

Axes are best treated as composite scene objects.

A single logical axis may own or derive:

1. one line contribution,
2. many tick-mark contributions,
3. many label contributions,
4. optional grid-line contributions,
5. optional linked annotation such as a colorbar.

This is similar in spirit to how other semantic scene objects may emit multiple family-specific
contributions into one `FramePlan`.


## Axis Domain Source

An axis needs a clear data-domain source.

That source may come from:

1. explicit user-provided limits,
2. limits derived from one or more visuals,
3. a linked multi-panel domain,
4. a shared normalization policy at scene level.

The source of truth should remain explicit so that tick generation is deterministic.


## Axis Scale Model

The first axis spec should at least allow:

1. linear scale,
2. log scale.

Other scales may be introduced later, but the key rule is:

1. scale policy belongs to scene semantics,
2. scale policy is not a backend concern.


## Tick Generation

Tick generation should be a scene-side algorithm working in `DataSpace`.

It should consider:

1. visible domain,
2. scale type,
3. target density,
4. label readability,
5. panel size or available screen extent.

The output should be deterministic for a given:

1. domain,
2. panel state,
3. tick policy.


## Label Formatting

Label formatting is also a scene-side concern.

Formatting should depend on:

1. original data values,
2. scale mode,
3. precision policy,
4. optional user formatting rules.

Formatting should not depend on low-level render-space coordinates.


## Axis Geometry Build

Once ticks are chosen and formatted, the scene layer should derive:

1. axis line geometry,
2. tick mark geometry,
3. label placements,
4. optional grid-line geometry.

This derived geometry should live in visual-ready coordinates and then follow the panel transform like
other renderable contributions.


## Relationship To `FramePlan`

Axes should participate in `FramePlan` as producers of derived contributions.

Typical `FramePlan` effects:

1. axis semantic changes may dirty derived axis resources,
2. tick recomputation may trigger `UploadNode` work,
3. axis line, tick, and label contributions may feed one or more `RenderNode`s,
4. offscreen or picking-aware annotation paths may add further nodes later.

`FramePlan` should see the result of axis derivation, not be forced to generate ticks from scratch.


## Relationship To Transform Pipeline

Axes are the clearest case where the transform split must remain explicit.

For axes:

1. semantic tick values are chosen in `DataSpace`,
2. tick geometry is placed in `VisualSpace`,
3. panel-local transforms move that geometry afterward.

This means axes are a direct consumer of `TRANSFORM_PIPELINE.md`.


## Relationship To Visual Families

Axes are not themselves ordinary visual families.

Instead:

1. axes are scene-side semantic objects,
2. they emit contributions to ordinary visual families,
3. those contributions should still satisfy the visual-family contracts they use.

This keeps the visual-family taxonomy clean while still supporting rich axis behavior.


## 2D Axes

The first axis spec should primarily target 2D panels.

For 2D axes:

1. the visible data range is well-defined from panzoom state,
2. tick generation is straightforwardly tied to the viewed domain,
3. axes are expected to update live with pan and zoom.


## 3D Axes

3D axes should be supported conceptually, but the first contract may remain simpler.

Likely 3D responsibilities:

1. represent orientation and scale in data-aware terms,
2. remain panel-aware under camera motion,
3. optionally expose reduced or simplified tick behavior compared with 2D axes at first.

The spec should avoid overfreezing 3D axis behavior before implementation pressure justifies it.


## Colorbars

Colorbars are closely related to axes, but they should not be conflated with ordinary x/y/z axes.

A good current model is:

1. a colorbar is a related semantic annotation object,
2. it may share tick generation and label formatting machinery with axes,
3. it may emit `image` plus `glyph` plus `segment` contributions,
4. it should remain scene-side and data-aware.


## Invalidations

Axis-related derived contributions should usually be regenerated when:

1. the visible data domain changes,
2. scale policy changes,
3. tick policy changes,
4. formatting policy changes,
5. panel size changes in ways that affect density or layout.

They should not necessarily be regenerated when:

1. unrelated visuals change,
2. unrelated scene resources change,
3. the current panel motion remains within the already covered domain and density tolerances.


## Rules

1. Tick selection happens in `DataSpace`.
2. Label formatting happens from data-space values.
3. Tick and label geometry are built in `VisualSpace`.
4. Panel transforms still apply after that geometry is built.
5. Axes are scene-side semantic objects, not backend concepts.
6. Axes should emit ordinary visual-family contributions rather than inventing a parallel render
   path by default.


## Follow-On Work

This document should eventually be followed by:

1. worked 2D axis examples,
2. explicit colorbar notes if needed,
3. future API sketches for binding axes to domain sources and panels.
