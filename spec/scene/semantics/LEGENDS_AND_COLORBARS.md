# Scene Legends And Colorbars

This document defines how legends and colorbars should work in the future scene layer.

Legends and colorbars are annotation-side semantic objects.

They are not ordinary visual families, and they are not backend widgets.


## Purpose

Legends and colorbars should:

1. expose the semantic mapping from visual encodings to human-readable meaning,
2. remain attached to scene and panel semantics rather than shader or pipeline details,
3. support shared and panel-local summarization of one or more visuals,
4. fit naturally into annotation layout, invalidation, export, and interaction,
5. stay compatible with future capability adaptation without leaking backend details upward.


## Position

Legends and colorbars sit between:

1. visual-family semantic mappings,
2. panel-attached annotation layout,
3. scene resources such as parameter blocks, sampled fields, and derived labels,
4. `FramePlan` contributions for text, guides, ramps, and layout boxes.

The intended flow is:

1. one or more visuals declare encodings that may need explanation,
2. the scene decides which legend or colorbar objects should exist,
3. those objects derive entries, ramps, labels, and placement,
4. the resulting annotation contributions enter the scene-level `FramePlan`.


## API Model

`DvzLegend` and `DvzColorbar` are distinct types at the API level. They share placement and
invalidation rules but have different content models and are not subtypes of a common base.

Legends and colorbars hold a reference to a `DvzScale*` handle (see `semantics/SCALES.md`). The scale is
a first-class scene object with a stable identity; legends and colorbars do not own the scale.


## Core Rule

Legends and colorbars should explain scene meaning, not rendering mechanics.

They should describe concepts such as:

1. category-to-color mapping,
2. marker-shape meaning,
3. line-style meaning,
4. scalar-domain-to-colormap mapping,
5. threshold or level meaning,
6. per-panel interpretation of the currently visible visual set.

They should not expose concepts such as:

1. descriptor bindings,
2. uniform layouts,
3. texture units,
4. shader specialization constants,
5. backend colormap implementation details.


## Non-Goals

This document does not define:

1. the final public C API,
2. the final theme or styling language,
3. the final text-layout engine,
4. the exact color-ramp tessellation or sampling method,
5. the exact DRP2 command sequences for legend or colorbar rendering.


## Why These Are Special

Legends and colorbars are not just decorative overlays.

They are semantic summaries of visual encodings.

That makes them different from ordinary visuals because:

1. they may summarize several visuals at once,
2. they may depend on visual variant and parameter state rather than bulk item data alone,
3. they may need panel-aware filtering or visibility decisions,
4. they often combine text, geometry, and sampled-ramp contributions,
5. they may need scene-level consistency across several panels.


## Relationship To Annotations

Legends and colorbars are annotation classes as defined in
[semantics/ANNOTATIONS.md](ANNOTATIONS.md).

They should inherit the same broad rules:

1. semantic ownership first,
2. explicit attachment scope,
3. explicit anchor and placement policy,
4. scene-side invalidation,
5. optional interaction and picking,
6. ordinary family contributions under the hood.


## Main Responsibilities

A legend or colorbar object should be responsible for:

1. knowing which visual mappings it summarizes,
2. deriving user-visible entries or ramps,
3. deriving labels and layout,
4. declaring whether it is panel-local or shared,
5. reacting to the correct invalidation triggers,
6. emitting ordinary scene contributions rather than backend-specific overlay behavior.


## Terminology Split

The scene spec should keep a clear distinction between:

1. legend,
2. colorbar,
3. scale object,
4. visual mapping.


### Legend

A legend is a discrete or semi-discrete explanatory object that lists named visual encodings.

Typical legend entries may explain:

1. categories,
2. line styles,
3. marker shapes,
4. marker sizes,
5. selected variants,
6. visibility states.


### Colorbar

A colorbar is a continuous or sampled explanatory object that shows how scalar or ordered values map
to color.

Typical colorbar contents may include:

1. one ramp,
2. labeled ticks,
3. units or title,
4. optional threshold markers,
5. optional out-of-range indication.


### Scale Object

A scale object is the semantic mapping being explained.

Examples:

1. categorical palette mapping,
2. continuous colormap domain,
3. size scale,
4. opacity scale,
5. symbol-shape mapping.

The final scene design may expose explicit scale objects or derive them from visual parameters, but
the semantic distinction should remain explicit in the spec.


### Visual Mapping

A visual mapping is the currently active relationship between scene data or parameters and what the
visual shows.

Legends and colorbars summarize visual mappings, not raw resources alone.


## Ownership And Attachment

Legends and colorbars should have explicit ownership scope.

The first useful scopes are:

1. panel-attached,
2. scene-shared,
3. visual-attached,
4. axis-attached.


### Panel-Attached

This should be the default.

A panel-attached legend or colorbar explains what the user sees in one panel.

This matters because:

1. different panels may show different subsets of visuals,
2. panels may use different view modes or variants,
3. one shared visual may still need panel-specific explanatory context.


### Scene-Shared

A scene-shared legend or colorbar may be reused across panels when the semantic mapping is identical.

This should be allowed when:

1. the same visuals and mappings are visible in several panels,
2. one export layout wants a shared consolidated legend,
3. linked panels intentionally share explanatory context.


### Visual-Attached

Some legends or colorbars logically belong to one visual.

Examples:

1. one colorbar for one scalar field visual,
2. one discrete legend for one categorical marker visual,
3. one style legend for one path family variant.

Even in this case, placement is usually still panel-attached.


### Axis-Attached

A colorbar may be semantically close to an axis.

Examples:

1. an image visual with a scalar-value colorbar,
2. a volume visual with transfer-function explanation,
3. an axis-side value ramp aligned with one domain interpretation.

This does not make a colorbar an axis, but it does justify a close semantic relationship.


## Source Of Truth

The source of truth for a legend or colorbar should be a scene-semantic mapping description.

That source may come from:

1. visual family parameters,
2. explicit scale objects,
3. visual variants,
4. attached categorical metadata,
5. axis-side domain interpretation,
6. selection or filtering state.

The source of truth should not be:

1. backend shader code,
2. pipeline internals,
3. texture objects without semantic range information.


## Mapping Identity

Legends and colorbars should aggregate semantic mappings, not merely similar-looking rendered output.

The first scene slice should therefore assume an explicit stable logical mapping identity such as
`ScaleMapping` or an equivalent concept.

That identity should capture at least:

1. the semantic quantity or category set being explained,
2. the mapping policy such as categorical palette, continuous colormap, size scale, or symbol map,
3. domain or category definitions,
4. any interpretation details that would change what the legend or colorbar means.

The final API may expose this as an explicit object or derive it from visual state, but the spec-level
identity rule should be stable.


## Discrete Versus Continuous Explanations

The scene spec should explicitly support both:

1. discrete explanatory objects,
2. continuous explanatory objects.


### Discrete

Discrete legends explain named categories or finite style states.

Examples:

1. species categories,
2. cluster ids,
3. selection state,
4. marker-shape encoding,
5. line-dash encoding.


### Continuous

Continuous colorbars explain ordered scalar mappings.

Examples:

1. intensity,
2. probability,
3. temperature,
4. elevation,
5. transfer-function domain.


### Mixed

The scene spec should also allow hybrid cases.

Examples:

1. a continuous ramp with threshold markers,
2. a categorical legend with an additional size-scale explanation,
3. a segmented colorbar with labeled bands.


## Multi-Visual Aggregation

One legend may summarize several visuals when that is semantically correct.

This should be allowed for:

1. several visuals sharing one categorical palette,
2. layered visuals that intentionally present one combined explanation,
3. linked panels that reuse one shared explanatory object.

But the scene should avoid implicit aggregation when meanings differ.

If two visuals use similar colors for different semantics, they should not silently collapse into one
legend.

The aggregation rule should be:

1. implicit aggregation is allowed only when the mapping identity is semantically identical,
2. visual resemblance alone is not enough,
3. explicit scene configuration may still request a shared explanatory object when several visuals are
   intentionally tied to the same semantic mapping.


## Entry Model For Legends

A legend should be able to express entries conceptually containing:

1. stable legend-entry identity,
2. human-readable label,
3. one or more sample marks,
4. optional group or visual association,
5. optional visibility or enabled state,
6. optional interaction affordances.

The sample mark may be represented internally by ordinary families such as:

1. `marker`,
2. `segment`,
3. `path`,
4. `glyph`,
5. `image`.

But this should stay an implementation consequence, not the primary semantic model.


## Ramp Model For Colorbars

A colorbar should be able to express conceptually:

1. stable colorbar identity,
2. domain minimum and maximum,
3. scale policy such as linear or log,
4. tick and label policy,
5. ramp orientation,
6. optional threshold or level markers,
7. optional units and title,
8. optional out-of-range policy.


## Relationship To Axes

Colorbars often behave like annotation-side companions to axes.

They share several needs with axes:

1. semantic domain awareness,
2. tick generation,
3. label formatting,
4. panel-aware layout,
5. occasional semantic regeneration rather than per-frame rebuild.

But a colorbar is not just an axis:

1. it also explains a color mapping,
2. it may need a ramp image or derived sampled field,
3. it usually summarizes one or more visual mappings rather than one panel coordinate axis.


## Layout And Placement

Legends and colorbars should keep anchor and placement separate.

Useful placement policies include:

1. docked to panel edge,
2. corner placement,
3. attached to axis side,
4. outside-panel layout region in export contexts,
5. shared multi-panel layout region,
6. callout-like attached placement.

Placement policy should remain semantic and target-aware rather than renderer-specific.


## Ordering And Grouping

Legend entries should allow stable ordering and grouping.

Important use cases include:

1. preserving user-specified category order,
2. grouping entries by visual,
3. grouping entries by semantic section,
4. hiding empty or inactive groups,
5. preserving deterministic export ordering.


## Visibility Policy

Legends and colorbars should support explicit visibility rules.

Examples:

1. always visible,
2. visible only when attached visuals are visible,
3. visible only in export,
4. hidden during interaction,
5. collapsed until expanded,
6. omitted when no meaningful mapping exists.


## Interaction Policy

Legends and colorbars may be passive or interactive.

The scene spec should allow:

1. passive explanatory display,
2. pickable entries,
3. toggling category visibility,
4. highlight-on-hover behavior,
5. linked filtering across visuals or panels.

If interaction is enabled, the resulting behavior should still route through scene controllers and
scene state mutation rather than bypassing the scene layer.


## Resource Expectations

Legends and colorbars may depend on several scene resource classes.

Typical inputs include:

1. `ParameterBlockResource`,
2. `SampledField`,
3. `DerivedField`,
4. small `ItemTable`-like derived resources for sample marks,
5. panel-local derived label and layout resources.

The important point is that they consume scene resource semantics rather than directly exposing DRP2
or backend objects.


## Transform Expectations

Most legends and colorbars are panel- or viewport-relative annotations rather than data-space
renderables.

But some of their semantics still originate in data-space or visual mappings.

The usual split should be:

1. semantic mapping is resolved from visual or scale state,
2. labels and ticks are derived from that semantic mapping,
3. final placement is usually panel-relative,
4. some axis-attached cases may align partially with data-domain interpretation.


## Invalidation Model

Legends and colorbars should participate in ordinary scene invalidation.

Typical invalidation sources include:

1. visual visibility changes,
2. visual variant changes,
3. scale-domain changes,
4. colormap or palette changes,
5. category-set changes,
6. label-format or units changes,
7. panel size or layout changes,
8. interaction state changes for interactive legends,
9. export-target policy changes.


## Cheap Versus Expensive Updates

The scene should distinguish between:

1. cheap placement-only updates,
2. entry or tick regeneration,
3. ramp regeneration,
4. full layout recomputation,
5. upload changes.

Examples:

1. moving a docked legend after panel resize may be mostly layout work,
2. changing only panel corner placement should not require semantic remapping,
3. changing a colormap domain may require new ticks and possibly new ramp data,
4. changing category membership may require entry rebuild.


## Export And Offscreen Behavior

Legends and colorbars are first-class export concerns.

The scene spec should allow:

1. export-only consolidated legends,
2. higher-resolution or differently placed export colorbars,
3. deterministic annotation output in headless rendering,
4. target-specific visibility policies.


## Capability Adaptation Pressure

Legends and colorbars create capability pressure without justifying backend leakage.

Important examples:

1. text rendering availability,
2. sampled-ramp rendering support,
3. picking support for interactive legends,
4. limits on dynamic annotation churn,
5. export target constraints.

If adaptation is required, the scene should choose an explicit semantic fallback.

Examples:

1. omit interactivity but keep explanation,
2. reduce gradient fidelity while preserving ticks and labels,
3. replace a complex grouped legend with a simplified static summary,
4. suppress optional decorative sample marks while keeping core semantic entries.


## Validation Pressure

Legends and colorbars should be validated at the scene layer before emission.

Important validation cases include:

1. a colorbar requested for a visual with no scalar mapping,
2. a legend entry source with missing category labels,
3. incompatible aggregation of visuals with different semantics,
4. log-scale colorbar with invalid domain,
5. interactive legend requested without required picking support,
6. axis-attached colorbar requested on an incompatible panel or visual mode.


## Relationship To Other Scene Docs

This document should be read alongside:

1. [ANNOTATIONS.md](ANNOTATIONS.md),
2. [AXES.md](AXES.md),
3. [../pipeline/RESOURCE_MODEL.md](../pipeline/RESOURCE_MODEL.md),
4. [VISUAL_CONTRACT.md](VISUAL_CONTRACT.md),
5. [../pipeline/TRANSFORM_PIPELINE.md](../pipeline/TRANSFORM_PIPELINE.md),
6. [../pipeline/INVALIDATION_AND_CACHING.md](../pipeline/INVALIDATION_AND_CACHING.md).


## What This Document Intentionally Leaves Open

This document intentionally does not freeze:

1. the final sample-mark templating system,
2. the final collision and packing algorithm for dense legends,
3. the final text and ramp rendering implementation strategy.


## Immediate Follow-Up

`validation/VALIDATION.md` covers legend/colorbar validation and cross-object semantic checks that
build on this document.
