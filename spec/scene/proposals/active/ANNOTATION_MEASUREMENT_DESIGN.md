> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-09`
> - **Purpose:** define the intended v0.4 annotation and measurement model for scale bars,
>   dimensions, bounding boxes, and mixed screen-space/world-space overlays.

# Annotation and Measurement Design

This note narrows the broader scene annotation discussion into the active measurement-oriented
requirements that should shape v0.4 text, transforms, picking, and axes.

For immediate implementation work, start with
[../slices/ANNOTATION_LABEL_SLICE.md](../../slices/ANNOTATION_LABEL_SLICE.md). Measurement annotations
should be implemented after the retained label and text rendering paths are active.


## Objective

Support scientific annotations that are actually useful in interactive and export workflows:

1. adaptive scale bars with units,
2. 2D dimensions with units,
3. 3D bounding box overlays,
4. 3D dimension annotations,
5. labels and callouts that can live either in screen space or world space.


## Existing Grounding In The Repo

There is already useful broader spec context:

1. annotation semantics in
   [spec/scene/semantics/ANNOTATIONS.md](../../semantics/ANNOTATIONS.md)
2. text direction in
   [TEXT_DESIGN.md](TEXT_DESIGN.md)
3. transform/controller direction in
   [TRANSFORM_CONTROLLER_DESIGN.md](TRANSFORM_CONTROLLER_DESIGN.md)
4. picking direction in
   [PICKING_DESIGN.md](../promoted/PICKING_DESIGN.md)

This note does not replace the larger annotation spec. It defines the active v0.4 decisions for the
measurement-heavy subset that you already need now.


## Core Recommendation

Measurement and annotation should be treated as a first-class scene family, not as ad hoc labels
attached manually to unrelated visuals.

Recommended architecture split:

1. semantic annotation objects at scene level,
2. derived geometric and text contributions at frame-plan level,
3. rendering through ordinary visual/resource paths where practical.

This keeps measurement logic explicit while still letting the renderer reuse common visual families.


## Why This Needs Its Own Design

Measurement overlays sit at the intersection of:

1. text,
2. transforms,
3. picking,
4. units/domain formatting,
5. transparency/overlay composition.

If this is left implicit, scale bars and dimensions will become one-off special cases embedded in
controllers or example code.


## Recommended Family Split

The active measurement problem is larger than plain text but narrower than a full GUI system.

Recommended conceptual families:

1. labels
   - text-first annotations
2. guides
   - lines, rulers, bounding boxes, callout leaders
3. measurements
   - scale bars and dimensions with unit-aware formatting

The public API does not need to expose these as three distinct opaque handle types immediately, but
the implementation should think in these terms.


## Placement Modes

Annotations need explicit placement semantics from the start.

Recommended placement classes:

1. screen-space
   - anchored to viewport/panel space
   - unaffected by model-space arcball
2. world-space
   - anchored to object/world geometry
   - transformed through model/view/projection
3. hybrid
   - semantic anchor in world/data space, final layout resolved in screen space

That third class matters for callouts where the target lives in 3D but the label placement behaves
more like an overlay.


## Adaptive Scale Bar

The adaptive scale bar should be treated as a first-class annotation primitive.

Recommended default behavior:

1. screen-space anchored, usually near a panel edge,
2. horizontal by default,
3. sized from a nice-value ladder derived from current view scale,
4. formatted with adaptive units such as `2 cm`, `5 mm`, `1 mm`,
5. updated automatically with pan/zoom or effective 3D projected scale changes.

Do not treat the scale bar as “just text plus one line”. It has semantic behavior and unit policy.


## Scale Bar Unit Policy

The scale bar needs a unit-aware formatting layer.

Recommended policy:

1. annotation stores a base unit system and conversion rules,
2. visible label is chosen from a nice-step sequence,
3. label formatting adapts unit prefixes when it improves readability,
4. the rendered segment length follows the chosen semantic length, not the other way around.

The visible unit formatting should be owned by the annotation/measurement layer, not buried inside
axes or text.


## Nice-Step Selection

For both scale bars and dimensions, use a standard nice-value ladder:

1. `1`
2. `2`
3. `5`
4. powers of ten around those anchors

This is the right baseline for:

1. scale bars,
2. tick-like measurement labels,
3. dimension readouts.

If you later want a `1 / 2 / 2.5 / 5` ladder for specific domains, that can be a formatting policy
variant rather than a different annotation architecture.


## 2D Dimensions

2D dimensions should be a real measurement type, not improvised from text plus line primitives.

Recommended baseline components:

1. extension lines,
2. dimension line,
3. arrowheads or ticks,
4. centered or offset text label,
5. unit-aware semantic length.

Recommended behavior:

1. dimension anchor points live in data or panel space,
2. text placement is derived automatically,
3. unit formatting follows the same measurement formatting rules as the scale bar.


## 3D Bounding Box Overlay

An antialiased 3D bounding box around an object should be a supported measurement/inspection tool.

Recommended behavior:

1. world-space overlay,
2. transformed with the object/model when attached,
3. rendered as an annotation helper rather than part of the mesh geometry,
4. optionally pickable at the annotation-object level,
5. capable of carrying dimension labels later.

This should not require mutating mesh geometry or generating a fake mesh asset just to show a box.


## 3D Dimension Annotations

3D dimensions are a separate requirement from simple labels.

Recommended baseline:

1. two 3D anchor points or one axis-aligned bounding-box extent,
2. derived dimension line and leaders in world space,
3. text label either in world space or hybrid screen-facing placement,
4. unit-aware length formatting,
5. optional billboarding for text readability.

This is one reason the text design must support both world-space text and screen-facing placement.


## Relationship To Text

Measurement annotations should consume the text system, not redefine it.

Recommended split:

1. measurement layer decides semantic value, unit formatting, and placement intent,
2. text layer renders glyph runs and simple backing/rule primitives,
3. guide/measurement geometry is generated separately from glyph layout.

This keeps the equation and general text backend reusable.


## Relationship To Mesh And Geometry

Measurements should not require geometry mutation.

Recommended rule:

1. annotations derive from scene objects, geometry bounds, or selected identities,
2. resulting lines/boxes/labels are separate scene contributions,
3. mesh resources remain reusable data assets and are not polluted with overlay geometry.


## Relationship To Picking

Picking must understand measurement objects explicitly.

Recommended behavior:

1. the measured target may itself be picked through ordinary visual picking,
2. annotation objects may also be pickable as annotation objects,
3. pick result kind should distinguish annotation object from mesh face or point item,
4. clicking a dimension or callout should not be forced through mesh pick ids.

For the first implementation, annotation-object-level picking is enough. Sub-primitive picking on a
dimension arrow or label is not required initially.


## Relationship To Controllers

Controller behavior should follow the transform design note.

Recommended categories:

1. screen-space scale bars: `FIXED`
2. world-space bounding boxes: `APPLY`
3. world-space dimensions tied to an object: `APPLY`
4. hybrid callouts: world anchor plus screen-space layout policy

This should stay explicit rather than inferred from the visual family.


## Relationship To Transparency And Overlay Composition

Annotation rendering needs deliberate placement in the frame plan.

Recommended rule:

1. screen-space helpers and scale bars render in explicit overlay stages,
2. world-space measurement lines and labels may participate in depth testing when that improves
   legibility,
3. overlay composition should stay explicit rather than piggybacking accidentally on transparent
   mesh passes.

This matters for WBOIT integration and for consistent export behavior.


## Resource Model Recommendation

The measurement layer should use scene resources, not immediate draw calls.

Likely derived resources:

1. dynamic line/segment geometry for guides and boxes,
2. dynamic glyph runs for labels,
3. optional background rectangles/rules,
4. unit-formatting metadata at the semantic annotation level.

The key point is that measurement semantics live above these resources; the resources are derived
rendering artifacts.


## Domain And Unit Ownership

Measurement features need a home for unit semantics.

Recommended ownership:

1. scenes or panels may carry default unit systems,
2. annotations may override unit formatting or conversion rules locally,
3. axes/domain logic and measurement logic should share common unit-formatting helpers rather than
   duplicate them.

This should be coordinated with the future axes/domain note, but the measurement layer should not
wait for a full axes implementation before becoming unit-aware.


## Export And Reproducibility

Measurement annotations often matter most in exported figures.

Recommended rule:

1. annotations should be scene-owned retained objects,
2. export should replay them deterministically,
3. dynamic helpers may opt out explicitly, but the default assumption should be that measurement
   annotations are part of the figure state.


## Initial Public API Direction

The exact signatures can still evolve, but the conceptual API should look like:

1. create annotation/measurement object,
2. set placement mode,
3. set semantic anchors or target object,
4. set measurement style and unit policy,
5. attach to one or more panels if needed.

Likely first public concepts:

1. `scale_bar`
2. `dimension`
3. `bbox_annotation`
4. generic label/callout

The public surface can expose typed constructors while still routing through shared internal
annotation machinery.


## Immediate Scope Recommendation

The narrowest useful first implementation slice is:

1. screen-space adaptive scale bar,
2. world-space 3D bounding box overlay,
3. world-space dimension label and line for one object extent,
4. annotation-object-level picking,
5. unit-aware formatting shared with future axes.


## Explicit Non-Goals For The First Slice

1. full CAD-grade dimensioning toolchains,
2. editable multi-paragraph annotation layout,
3. rich leader-routing optimization,
4. per-glyph picking,
5. a complete legend/colorbar system in the same first pass.
