# Screen-Space Overlay Layout Proposal

> **Status:** Exploratory proposal.
> **Scope:** minimal retained overlay layout for labels, cards, readouts, legends, and dashboard
> metadata panels.
> **Primary pressure tests:** `../../examples/dashboards/SEMANTIC_EMBEDDING_ATLAS.md`,
> `../../examples/dashboards/IMAGE_EMBEDDING_LOD.md`, linked probe/colorbar examples, and dashboard
> telemetry.


## Summary

Datoviz examples increasingly need small screen-space UI elements: hover readouts, selected-item
cards, pinned probe panels, legends, label groups, and telemetry. These should not require bundling
HTML/CSS or turning scene into a full GUI framework. The first Datoviz-native overlay layout should
be deliberately small: anchored boxes, vertical stacks, rows, text, optional images, backgrounds,
borders, and clipping.

The proposal is to add a retained screen-space overlay layer that emits ordinary scene visuals and
frame-plan commands. The layout system computes pixel rectangles and z ordering; rendering remains
inside the scene -> DRP2 -> runtime path.


## Non-Goals

This proposal does not define:

1. a DOM;
2. CSS selectors, cascading styles, or layout inheritance;
3. arbitrary flexbox/grid behavior;
4. text editing widgets;
5. native menus or dialogs;
6. a replacement for Dear ImGui integration;
7. a general application UI framework.

External GUI systems remain valid for applications that need full controls. The overlay layout is
for lightweight visualization-attached explanatory objects.


## Core Concepts

An overlay owns one or more anchored boxes in screen coordinates:

```text
overlay
  card/top-right
    title text
    row label/value
    row label/value
    optional image slot
  card/bottom-left
    telemetry rows
```

Minimum retained objects:

```text
DvzOverlay       overlay layer attached to a figure or panel
DvzOverlayBox    anchored rectangle with padding, gap, background, and clipping
DvzOverlayText   one text run or fixed wrapped text block
DvzOverlayRow    two-column label/value row
DvzOverlayImage  optional screen-space image or icon slot
```

The implementation may start internal or example-local. Public API should wait until two or more
examples prove the shape.


## Layout Model

The first layout model should support:

1. anchors: top-left, top-right, bottom-left, bottom-right, center, cursor-relative;
2. fixed or content-derived width with min/max constraints;
3. vertical stack layout;
4. row layout with label and value columns;
5. padding and gap in physical pixels;
6. optional clipping to the overlay box;
7. z ordering above data visuals and below external GUI overlays;
8. high-DPI scaling through the same figure/panel pixel-space convention as other screen-space
   scene objects.

The layout should not scale font sizes with viewport width. Text should wrap or elide inside the
box instead of overflowing. A selected card may grow downward from the top-right anchor, but it
should not shift the main data panel or mutate panel layout.


## Rendering Model

The overlay should compile to existing or planned scene visuals:

1. rectangle/primitive visuals for backgrounds and borders;
2. text visuals for titles, rows, and labels;
3. image visuals for thumbnails or icons;
4. optional path/primitive visuals for callout lines.

Overlay resources should be retained. Updating a selected card should update text, colors, and
small buffers, not rebuild pipelines, descriptor layouts, or sampled fields. If text rendering is
not yet available, examples may keep overlay/card support as retained bookkeeping plus stdout
validation until glyph rendering lands.


## Coordinate And Lifetime Rules

Overlay coordinates are screen-space pixels after panel layout has resolved. An overlay attached to
a panel is clipped to that panel by default. An overlay attached to a figure may occupy the full
window and must respect figure resize.

Overlay boxes own their layout state and borrowed scene resources must be explicit. Destroying an
overlay should remove or hide every visual it created. Repeated selection changes should reuse
existing overlay visuals whenever the number of visible rows stays within the retained capacity.


## Example Card

The semantic embedding atlas selected-item card could contain:

```text
Machine learning
en.wikipedia.org/wiki/Machine_learning

Cluster    AI / statistics
Subset     20231101.en
Nearest    Artificial intelligence
           Statistical learning theory
           Pattern recognition
```

The image embedding LOD card could contain:

```text
PD12M item 183204
Caption    A public-domain botanical illustration...
License    CC0
Source     Wikimedia Commons
LOD        32x32
```


## API Sketch

The first public shape should stay small if it is promoted:

```c
DvzOverlay* dvz_overlay(DvzPanel* panel);
DvzOverlayBox* dvz_overlay_card(DvzOverlay* overlay, DvzOverlayAnchor anchor);
void dvz_overlay_box_padding(DvzOverlayBox* box, float x, float y);
void dvz_overlay_box_max_width(DvzOverlayBox* box, float width_px);
void dvz_overlay_title(DvzOverlayBox* box, const char* text);
void dvz_overlay_row(DvzOverlayBox* box, const char* label, const char* value);
void dvz_overlay_text(DvzOverlayBox* box, const char* text);
void dvz_overlay_clear(DvzOverlayBox* box);
void dvz_overlay_visible(DvzOverlayBox* box, bool visible);
```

This is only a pressure-test sketch. The first implementation may use a private helper in examples
or scene internals until text rendering, clipping, and retained update semantics are stable.


## Validation

Validation should cover:

1. card appears at the expected anchor after resize;
2. text stays within the box bounds;
3. hidden cards do not draw stale content;
4. selection updates reuse retained resources;
5. panel-attached overlays clip to the panel;
6. high-DPI sizes remain stable;
7. a screenshot/readback smoke can detect nonblank background and text pixels when glyph rendering
   is active.


## Open Questions

1. Should overlays be owned by figures, panels, or both?
2. Should cards reserve retained row capacity to avoid allocation during selection changes?
3. Should row value text wrap, elide, or scroll when too long?
4. Should cursor-relative hover cards be separate from anchored persistent cards?
5. Should external GUI backends be able to consume the same overlay model, or should this remain
   scene-only?
