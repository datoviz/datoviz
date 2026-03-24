# Scene Annotations

This document defines how annotations should work in the future scene layer.

Annotations are scene-side semantic objects.

They are not merely decorative draw calls, and they are not backend concepts.


## Purpose

Annotations should:

1. attach explanatory, diagnostic, or interactive context to scene content,
2. remain expressible in scene semantics rather than renderer mechanics,
3. support both static explanatory overlays and live interaction-driven overlays,
4. fit naturally into panel-local transforms, picking, and invalidation,
5. emit ordinary family contributions or panel-local overlay contributions as needed.


## Position

Annotations sit between:

1. scene-owned semantic objects such as panels, visuals, axes, and resources,
2. panel-local interaction and controller state,
3. derived visual contributions and `FramePlan`,
4. optional picking and readback interpretation.

The intended flow is:

1. scene objects or controllers define annotation intent,
2. the scene derives annotation content and placement,
3. annotation contributions are assembled into the current `FramePlan`,
4. the runtime only sees planned render work and optional picking work.


## Core Rule

Annotations should be modeled by semantic role, not by low-level rendering technique.

That means the scene should express concepts such as:

1. label,
2. guide line,
3. probe readout,
4. crosshair,
5. overlay callout,
6. legend entry,
7. colorbar companion.

It should not force the user to think in terms of:

1. glyph buffers,
2. segment batches,
3. texture quads,
4. backend overlay passes.


## Non-Goals

This document does not define:

1. the final public C API,
2. the final text shaping or font pipeline,
3. the exact overlay pass topology,
4. the final styling vocabulary,
5. the exact DRP2 command sequence for annotation rendering.


## Why Annotations Are Special

Annotations differ from ordinary visuals because they often depend on semantic context in addition to
renderable data.

Typical dependencies include:

1. the current panel-local view,
2. hover or selection state,
3. axis-derived semantic values,
4. sampled or queried data values,
5. layout collision or anchoring rules,
6. panel-edge or viewport-relative placement.

This means annotations often sit across:

1. scene semantics,
2. panel-local viewing state,
3. interaction state,
4. derived geometry and text layout.


## Main Responsibilities

An annotation object or annotation-producing scene object should be responsible for:

1. defining the semantic meaning of the annotation,
2. defining its anchor source,
3. defining its placement policy,
4. deriving its visible content,
5. producing ordinary family contributions or overlay contributions,
6. reacting to the correct invalidation triggers.


## Annotation Classes

The scene spec should recognize at least the following conceptual classes:

1. labels,
2. guides,
3. probes,
4. crosshairs,
5. callouts,
6. overlays,
7. legends,
8. colorbars.


### Labels

Labels are text-oriented annotations attached to one semantic target.

Examples:

1. point labels,
2. panel titles,
3. axis-adjacent labels,
4. selection labels,
5. static explanatory text.


### Guides

Guides are geometric annotations intended to help reading or alignment.

Examples:

1. horizontal or vertical guide lines,
2. threshold markers,
3. bounding boxes,
4. region outlines,
5. reference planes in 3D-oriented panels.


### Probes

Probes are interaction-aware readouts tied to a queried semantic target.

Examples:

1. hover value readout,
2. cursor-aligned sampled value display,
3. nearest-item summary,
4. live slice or voxel inspection.


### Crosshairs

Crosshairs are panel-local cursor-aligned guides, often coupled to probes or linked panels.

They usually depend on live interaction state rather than static scene resources.


### Callouts

Callouts attach explanatory text or highlights to a target anchor.

They commonly combine:

1. one anchor target,
2. one connecting guide,
3. one text block or badge,
4. optional emphasis styling.


### Overlays

Overlays are annotations positioned primarily in panel or viewport coordinates rather than data
coordinates.

Examples:

1. corner labels,
2. status badges,
3. export stamps,
4. viewport-relative HUD text.


### Legends And Colorbars

Legends and colorbars are annotation families large enough to deserve their own follow-up document.

For the purposes of this document:

1. they are annotations,
2. they are usually panel-attached rather than visual-family primitives,
3. they often summarize mappings used by one or more visuals,
4. they may emit multiple derived contributions.


## Attachment Scope

Annotations should have explicit ownership or attachment scope.

The minimum useful scopes are:

1. scene-global,
2. panel-attached,
3. visual-attached,
4. axis-attached,
5. interaction-derived.


### Scene-Global

These annotations are owned by the scene and may appear in one or more panels.

Examples:

1. shared title or subtitle objects,
2. shared probe state mirrored across panels,
3. global explanatory overlays.


### Panel-Attached

These annotations belong to one panel.

Examples:

1. panel title,
2. panel-local crosshair,
3. corner status overlay,
4. panel-local legend.


### Visual-Attached

These annotations derive meaning from one visual instance.

Examples:

1. labels for selected points,
2. a threshold guide owned by one visual,
3. a visual-specific readout or highlight.


### Axis-Attached

These annotations derive from or extend axis semantics.

Examples:

1. axis titles,
2. domain markers,
3. a linked colorbar,
4. axis-side probes or rulers.


### Interaction-Derived

These annotations are ephemeral and controller-driven.

Examples:

1. hover tooltip,
2. drag-selection rectangle,
3. live cursor probe,
4. transient measurement guide.


## Anchor Model

Every annotation should define where its meaning is anchored.

Useful conceptual anchor kinds include:

1. data-space anchor,
2. visual-space anchor,
3. panel-space anchor,
4. viewport-relative anchor,
5. item or group identity anchor,
6. sampled-value anchor.


### Data-Space Anchor

The annotation is anchored to a semantic data coordinate or domain value.

Examples:

1. a threshold line at `x = 3.2`,
2. a label for one data point,
3. a marker at one isovalue.


### Visual-Space Anchor

The annotation is anchored after data normalization but before panel-local viewing transforms.

This is useful when:

1. a visual has already defined a normalized layout,
2. the annotation should track that layout,
3. the original semantic data value is not the primary placement source.


### Panel-Space Anchor

The annotation is anchored in panel-local coordinates.

Examples:

1. crosshair lines,
2. drag boxes,
3. panel-relative probe readouts.


### Viewport-Relative Anchor

The annotation is anchored relative to panel edges or corners.

Examples:

1. top-left panel title,
2. bottom-right scale readout,
3. corner legend placement.


### Identity Anchor

The annotation is anchored to a picked or selected item or group identity rather than to one raw
coordinate alone.

This matters for grouped visuals and for annotations that must survive replanning.


## Placement Model

The scene spec should keep anchor and placement distinct.

An annotation may know what it refers to before the scene decides exactly where it should be drawn.

Placement policy should therefore cover:

1. direct placement on the anchor,
2. offset placement from the anchor,
3. edge docking,
4. collision-avoiding placement,
5. leader-line or callout placement,
6. panel-relative stacking.


## Relationship To The Transform Pipeline

Annotations should follow the same transform discipline as the rest of the scene model.

The important split is:

1. annotation meaning may originate in `DataSpace`,
2. annotation geometry or text anchors may be derived in `VisualSpace`,
3. panel-local viewing transforms may move those contributions afterward,
4. viewport-relative overlays may bypass `DataSpace` entirely and live in panel or viewport space.

This means not every annotation needs the same transform path.


## Annotation Families Versus Underlying Contributions

Annotations are not required to correspond one-to-one with a primitive visual family.

One logical annotation may emit several contribution types.

Examples:

1. a label may emit `glyph` plus a background box,
2. a crosshair may emit two `segment` contributions,
3. a probe may emit `glyph`, `segment`, and highlight-marker contributions,
4. a callout may emit guide geometry plus text plus emphasis markers.

This is the same composite pattern already used by axes.


## Text And Geometry Expectations

The scene layer should be free to realize annotations through ordinary families such as:

1. `glyph`,
2. `segment`,
3. `path`,
4. `marker`,
5. `image`.

This should remain an implementation consequence, not the public semantic model.


## Interaction Coupling

Some annotations are static and some are interaction-driven.

The spec should explicitly support:

1. annotations that persist until user code removes them,
2. annotations that appear only while a controller state is active,
3. annotations updated by picking results,
4. annotations mirrored across linked panels.

This keeps annotation logic aligned with `PICKING.md` and `CONTROLLERS.md`.


## Picking Expectations

Annotations may themselves participate in picking, but they should do so by scene identity.

The scene should be free to mark an annotation as:

1. non-pickable,
2. pickable as one annotation object,
3. pickable with sub-part identity when needed.

Examples:

1. clicking a legend entry filters one visual,
2. clicking an annotation badge opens details,
3. hovering a callout highlight reveals more context.


## Invalidation Model

Annotations should integrate with scene invalidation rather than bypass it.

Typical invalidation sources include:

1. content change,
2. anchor change,
3. placement-policy change,
4. panel transform change,
5. interaction-state change,
6. picking result arrival,
7. attached visual or axis state change,
8. panel layout or size change.


## Cheap Versus Expensive Updates

The scene should distinguish between:

1. cheap movement of already-derived annotation contributions,
2. semantic content rebuild,
3. layout recomputation,
4. upload changes.

Examples:

1. a viewport-corner title may only need panel-size-aware layout updates,
2. a crosshair may move every frame without semantic rebuild,
3. a probe tooltip may need content regeneration when the sampled value changes,
4. collision-avoiding labels may require a more expensive layout pass after resize or zoom changes.


## Shared Versus Panel-Local Resources

Annotations should be allowed to reuse shared scene resources when the semantics allow it.

But the model must also allow panel-local derived resources for:

1. viewport-relative layout,
2. panel-local text placement,
3. picking overlays,
4. linked-panel mirror state,
5. export-only overlays.

Panels should not duplicate scene resources unnecessarily, but panel-local annotation derivation is
often correct.


## Export And Offscreen Behavior

Annotations should participate naturally in offscreen and export-oriented scenes.

The scene spec should allow:

1. export-only annotations,
2. annotations omitted from interactive display but included in export,
3. deterministic probe or stamp overlays for reproducible output,
4. annotation visibility policies that differ by target mode.


## Capability Adaptation Pressure

Annotations create pressure on runtime capabilities without justifying backend leakage.

Important pressures include:

1. text rendering availability or quality,
2. offscreen readback support,
3. picking support,
4. layered or overlay-like composition support,
5. resource limits for dynamic annotation churn.

If capability gaps matter, the scene layer should adapt through explicit policy rather than through
backend-specific escape hatches.


## Relationship To Other Scene Spec Documents

This document should be read alongside:

1. `AXES.md` for axis-derived labels and related semantic guides,
2. `PICKING.md` for interaction-driven probes and pickable annotation identity,
3. `CONTROLLERS.md` for controller-driven transient overlays,
4. `TRANSFORM_PIPELINE.md` for anchor and placement transform stages,
5. `INVALIDATION_AND_CACHING.md` for dirty-scope and reuse behavior,
6. `FRAME_PLAN_IR.md` for how annotation contributions enter one frame plan.


## What This Document Intentionally Leaves Open

This document intentionally does not freeze:

1. whether annotations are represented by one generic object type or several specialized types,
2. the final style or theme object model,
3. the final text-layout engine boundary,
4. the final collision-avoidance algorithm,
5. whether legends and colorbars remain a shared annotation superclass or become dedicated scene
   objects.


## Immediate Follow-Up

The next documents enabled by this one should be:

1. `LEGENDS_AND_COLORBARS.md`,
2. `SCENE_VALIDATION.md`,
3. more worked examples with annotation-heavy and multi-panel cases.
