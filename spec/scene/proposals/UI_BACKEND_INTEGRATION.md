> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-09`
> - **Purpose:** define the active implications of external UI frameworks and host backends for the
>   scene boundary, including Dear ImGui and Qt/PyQt-style embedding.

# UI and Backend Integration

This note narrows the broader external-UI discussion into the active design decisions that matter
for scene mutation, embedding, and host-toolkit integration.


## Objective

Support application workflows where external UI frameworks and host toolkits coexist with the scene
system, including:

1. Dear ImGui property panels and dockable tools,
2. UI sliders and inspectors mutating scene-owned data,
3. embedded Datoviz panels inside a Qt/PyQt-style application,
4. future tool windows showing scene textures or offscreen panels.


## Existing Grounding In The Repo

Useful existing context:

1. external UI design:
   [spec/scene/integration/EXTERNAL_UI.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/integration/EXTERNAL_UI.md)
2. current window/backend hook:
   [include/datoviz/window/backend.h](/home/cyrille/GIT/Viz/datoviz/include/datoviz/window/backend.h)

Notably, the tree already exposes:

1. `dvz_window_register_qt_backend(...)`

So Qt-family hosting is not an entirely hypothetical direction.


## Core Recommendation

External UI frameworks and host toolkits remain outside the scene object model, but the scene API
must be explicitly designed to be a good mutation target for them.

Recommended split:

1. UI frameworks own widgets, docking, and application-level interaction,
2. scene owns visualization semantics and retained state,
3. runtime/window backends own embedding and presentation details,
4. scene-facing APIs should remain host-agnostic and should not embed host-specific object types or
   backend-native handles.

This keeps the scene model clean while still making real apps practical.


## Dear ImGui

Dear ImGui should remain an external-UI integration path, not a scene family.

Recommended implications for scene design:

1. scene-owned state must be mutable through stable setters and retained resource APIs,
2. colorbars, axes, labels, and picked highlights remain scene semantics,
3. ImGui widgets mutate that state but do not replace scene-native concepts,
4. texture display inside ImGui should use logical handles, not backend-native image views in scene
   code.

This matches the current direction of `integration/EXTERNAL_UI.md`.


## Docking And Movable Panels

Dockable dialogs and movable application windows should not pressure the scene into becoming a GUI
toolkit.

The actual scene implications are narrower:

1. panel layout must be mutable safely,
2. panel viewports may need to follow host-UI layout,
3. offscreen panel textures may need to be displayed inside host widgets,
4. scene redraw and invalidation must tolerate frequent property-panel changes.


## External UI Mutating Scene-Owned Data

This is one of the most important active requirements.

Recommended rule:

1. sliders, checkboxes, trees, and inspectors mutate scene-owned resources or parameters,
2. the scene performs dirty tracking and validation,
3. the next frame reflects the change through the normal retained pipeline,
4. no special “UI-only mutation path” should exist,
5. offscreen or embedded presentation handles exposed upward should remain logical scene/runtime
   handles rather than native backend objects.

Examples:

1. opacity slider mutates mesh visual material state,
2. slice-position slider mutates volume slice state,
3. color-range widget mutates a scene-owned scale,
4. visibility tree mutates scene-owned region visibility state.


## Input Routing

External UI should be allowed to consume input before scene controllers when appropriate.

Recommended rule:

1. host UI gets first chance to claim widget-oriented input,
2. unconsumed input continues to scene-native routing,
3. scene controllers remain responsible for panel-native interactions.

This is especially important for ImGui and Qt widget stacks.


## Qt / PyQt6 Direction

Qt-family embedding should be treated as a runtime/window-host concern, not a scene concern.

What is already visible in the repo:

1. there is a Qt backend registration hook at the window layer

What this implies:

1. scene should not assume GLFW is the only host,
2. scene should not assume it owns the top-level native window,
3. panel embedding and resize/event propagation must work through abstract runtime/window contracts.

For PyQt6 specifically:

1. the scene spec should remain agnostic,
2. the runtime/window layer should be able to host Datoviz content inside a Qt application,
3. scene mutation from Python-side UI callbacks should use the same retained scene APIs as any
   other external UI.


## Texture And Panel Embedding

One common integration pattern is rendering a Datoviz panel offscreen and showing it in host UI.

Recommended implications:

1. offscreen panel outputs should be expressible as logical scene/runtime handles,
2. host UI integrations should consume those handles through runtime-specific glue,
3. the scene should not expose backend-native texture descriptors in its public API.

This applies to both ImGui image widgets and Qt/PyQt embedding paths.


## DPI And UI Scaling

External UI integration increases pressure on DPI consistency.

Recommended rule:

1. scene text and panel coordinates remain in logical units,
2. host toolkit DPI changes propagate through runtime-scale updates,
3. scene text, overlays, and panel sizing remain coherent under host-UI scaling changes.

This ties directly into the updated text and high-DPI notes.


## Scene Implications Summary

The active scene-facing implications are:

1. safe retained mutation from external callbacks,
2. stable logical resource and texture handles,
3. panel layout mutability,
4. host-agnostic event and DPI routing,
5. no backend-native leak into public scene APIs.


## Immediate Scope Recommendation

The active implementation work should assume:

1. external UI is real and important,
2. ImGui remains a supported native overlay/tool path,
3. Qt/PyQt-style hosting should remain possible through the window/runtime boundary,
4. scene APIs must be clean mutation targets for these environments.


## Explicit Non-Goals For This Note

1. specifying a full Qt or PyQt backend implementation,
2. turning scene into a widget toolkit,
3. moving ImGui rendering into scene-owned DRP2 semantics,
4. defining Python bindings in this note.
