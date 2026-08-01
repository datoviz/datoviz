# Panel View Architecture

Status: implemented v0.4 panel View2D ownership and resolver contract. Updated: 2026-08-01.

## Ownership

1. Panel layout owns panel rect, plot rect, reserve, padding, viewport, and scissor.
2. Panel domains own requested semantic bounds and fit-to-data state.
3. Panel View2D policy owns aspect/framing behavior and resolver-derived fitted and visible domains.
4. Controllers own navigation and gesture state; they consume a panel-provided base extent and do not own viewport aspect.

The implemented resolver remains the source of truth:

```text
panel domains + plot rect + View2D policy + controller state
    -> base VIEW extent
    -> fitted DATA domains
    -> visible VIEW extent
    -> visible DATA domains
    -> DATA-to-VIEW model
```

## Implemented API Direction

`DvzPanelView2DDesc` and the panel View2D mutators express policy without duplicating domain or controller state. Contain plus equal aspect expands one visible domain so one X and Y data unit use the same plot-pixel scale. Resize and reserve changes rerun the resolver. Plot-clipped draws use plot viewport and scissor together.

Compatibility-era fit names and controller-owned aspect state are not part of the v0.4 contract. Public mutators return `DvzResult` according to the repository-wide recoverable-error convention.

## Deferred

Cover/crop framing, additional inversion/log policy, and broader public rendered-bounds APIs remain deferred until a concrete consumer justifies them. Do not add a second View2D resolver or store derived fitted domains as independent user state.
