# Scene Axes

Status: normative v0.4 scene semantics spec.

Axes are panel-owned semantic scene objects. They generate ordinary visual-family contributions, but
they are not ordinary visual families or backend concepts.


## Active Implementation Status

The active first 2D slice includes finite linear X/Y panel domains, panzoom-aware visible-domain
queries, data-to-visual mapping helpers, panel-owned axis handles, cached linear tick generation,
primitive-backed spine/tick/grid geometry, focused scene tests, and a `scatter_axes` example.

Rendered tick labels and axis labels remain text/layout follow-up. Log, inverted, categorical,
datetime, and 3D scientific axes are deferred unless explicitly activated.


## Purpose

Axes expose data-space semantics, generate ticks and labels from original data coordinates, track
panel navigation, and emit renderable scene contributions.


## Core Rules

1. Tick selection happens in `DataSpace`.
2. Label formatting happens from data-space values.
3. Tick and label geometry are built in `VisualSpace`.
4. Panel transforms apply after geometry is built.
5. Axes emit ordinary visual-family contributions instead of inventing a parallel render path.
6. Axes are owned by panels; free-standing axis objects are not supported.
7. Panel linking is achieved by sharing controllers, not by axis-link APIs.


## Axis Pipeline

| Step | Space/owner |
|---|---|
| determine visible domain | panel controller + bound domain |
| choose tick values | `DataSpace` |
| format labels | `DataSpace` values and scale policy |
| map tick anchors | `DataSpace` -> `VisualSpace` |
| build spine/tick/grid/label contributions | derived resources |
| render/live move | panel transform |

Each tick conceptually carries both its semantic data value and its visual-space anchor.


## Components And Contributions

A logical axis may derive spine lines, major/minor tick marks, labels, axis title, grid lines, and
related annotation/colorbar companions. Contributions may use `segment`, `glyph`, `path`, `image`,
or other ordinary families while still satisfying those family contracts.


## Ownership, Dimension, And Domain

| Concept | Rule |
|---|---|
| owner | panel owns default and custom axes |
| default axes | 2D panels create X and Y axes; 3D panels may create X/Y/Z |
| dimension | declared at creation and fixed |
| active axis count | each panel dimension has at most one primary active axis |
| primary domain | bound `DvzDataDomain` from [TRANSFORM_PIPELINE.md](../pipeline/TRANSFORM_PIPELINE.md) |
| secondary domain | dual-axis domains bind to per-visual override domains |

Axis scale (`DVZ_SCALE_LINEAR` or `DVZ_SCALE_LOG`) and direction (normal or inverted) come from the
bound domain. Tick generation and formatting must honor both.


## Visible Domain Selection

Within its bound domain, an axis selects the visible portion by priority:

| Mode | Behavior |
|---|---|
| explicit axis override | user-set axis min/max suppress panzoom tracking |
| panzoom-linked | default; query controller visible range and invert normalization to data units |
| fit-to-data | one-time initialization that sets controller/domain state, not a live binding |

An axis does not subscribe to panzoom events. During the frame update, it queries the panel
controller for the current visible domain and decides whether the cached layout remains valid.


## Regeneration Policy

Axes separate cheap live motion from semantic regeneration.

| Domain | Meaning |
|---|---|
| `visible_data_domain` | current panel/controller view |
| `covered_data_domain` | larger data range for which ticks/labels are already prepared |

If the visible domain stays inside the covered domain and density/readability invariants hold, reuse
derived resources and let panel transforms move them. Regenerate only when coverage, tick density,
panel size, scale, formatter, or label layout policy no longer satisfies the current view.

This makes panzoom/camera motion a possible cause of `AxisLayoutDirty`, but not a per-frame
regeneration trigger. Dirty-scope details are canonical in
[`../pipeline/INVALIDATION_AND_CACHING.md`](../pipeline/INVALIDATION_AND_CACHING.md).


## Tick And Label Rules

Tick generation is deterministic for a given domain, panel state, scale type, target density, label
readability policy, and tick policy. Formatting depends on original data values, scale, precision,
and optional user formatters, not low-level render coordinates.

The first required scale models are linear and log. Categorical, datetime, polar, geographic, and
full 3D placement behavior are deferred until implementation pressure justifies their contracts.


## 2D, 3D, And Colorbar Relationship

| Object | Contract |
|---|---|
| 2D axes | first target; visible data range comes from panzoom |
| 3D axes | conceptual support; camera-aware orientation/scale and possibly simplified ticks |
| colorbars | related annotation objects for scalar-to-color mappings; see [LEGENDS_AND_COLORBARS.md](LEGENDS_AND_COLORBARS.md) |


## Panel Linking

Controllers are first-class scene objects. Panels that share a controller automatically show
synchronized axes for the linked dimension. Partial linking uses shared controllers per dimension
(for example shared X with independent Y). The accepted controller handle model is recorded in
[`../decisions/CONTROLLER_BINDING_MODEL.md`](../decisions/CONTROLLER_BINDING_MODEL.md).


## FramePlan And Diagnostics

`FramePlan` consumes derived axis resources; it should not generate ticks from scratch. Axis
diagnostics should report bound domain, visible/covered domain, tick policy, scale, formatter,
whether the layout cache was reused, and why regeneration occurred.
