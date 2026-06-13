# Scene Axes

Status: normative v0.4 scene semantics spec.

Axes are panel-owned semantic scene objects. They generate ordinary visual-family contributions, but
they are not ordinary visual families or backend concepts.


## Active Implementation Status

The active first 2D slice includes finite linear X/Y panel domains, panzoom-aware visible-domain
queries, data-to-visual mapping helpers, panel-owned axis handles, cached linear tick generation,
primitive-backed spine/tick/grid geometry, rendered tick labels and axis labels through the current
text visual path, focused scene tests, and a `scatter_axes` example.

Label collision avoidance, richer formatter policy, log, inverted, categorical, datetime, and 3D
scientific axes are deferred unless explicitly activated.


## Purpose

Axes expose data-space semantics, generate ticks and labels from original data coordinates, track
panel navigation, and emit renderable scene contributions.


## Core Rules

1. Tick selection happens in `DataSpace`.
2. Label formatting happens from data-space values.
3. Grid geometry is built in resolved view/data coordinates and plot-clipped.
4. Ticks, spines, and labels are fixed overlay geometry positioned in panel-local coordinates after
   projecting tick anchors through the resolved panel view.
5. Axes emit ordinary visual-family contributions instead of inventing a parallel render path.
6. Axes are owned by panels; free-standing axis objects are not supported.
7. Panel linking is achieved by sharing controllers, not by axis-link APIs.


## Axis Pipeline

| Step | Space/owner |
|---|---|
| determine visible domain | panel view resolver output |
| choose tick values | `DataSpace` |
| format labels | `DataSpace` values and scale policy |
| map tick anchors | `DataSpace` -> resolved `ViewSpace` -> fixed panel interval |
| build spine/tick/grid/label contributions | derived resources |
| render/live move | panel transform |

Each tick conceptually carries both its semantic data value and its resolved view-space anchor.


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

Within its bound source domain, an axis selects the visible portion by priority:

| Mode | Behavior |
|---|---|
| explicit axis override | user-set axis min/max suppress panzoom tracking |
| panzoom-linked | default; consume the resolved visible DATA domain from the panel view resolver |
| fit-to-data | one-time initialization that sets controller/domain state, not a live binding |

An axis does not subscribe to panzoom events. During the frame update, it queries the panel view
resolver for the current fitted and visible domains and decides whether the cached layout remains
valid. Equal-aspect view fit must not be observed as a mutation of the source panel domain.


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

Grid lines should extend slightly beyond the resolved visible plot extent and rely on plot clipping
instead of endpoint equality at the plot boundary. Tick and label placement that must align with a
grid line should project the tick value through DATA -> VIEW and then map that view coordinate back
into the fixed panel overlay interval.

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

## Remaining Polish Queue

Keep near-term axes work focused on the 2D linear path before adding new scale families:

1. preserve the deterministic 1/2/5 tick ladder and formatter policy in tests;
2. add label collision handling, edge clipping, and panel-edge reserve before richer styling;
3. decide inverted-domain semantics before log or nonlinear axes become public;
4. let linked panels share domains and controllers while keeping per-panel axis layout caches;
5. add richer text style only through the shared text/label stack;
6. defer categorical, datetime, polar, geographic, and full 3D axes until the linear cache and
   invalidation model is stable.
