# GUI Viewports

This note describes the active v0.4 Dear ImGui integration shape.


## Terms

`DvzGui` is the Dear ImGui overlay attached to a visible GLFW `DvzAppWindow`.

`DvzGuiViewport` is an ImGui-hosted Datoviz viewport. It displays the latest image rendered by an
offscreen Datoviz app-window inside an ImGui window. It is not a scene `DvzPanel`.

`DvzPanel` remains the scene layout primitive inside a `DvzFigure`. A figure rendered in a GUI
viewport may contain one or many scene panels.


## Creation

The preferred public path is:

```c
DvzGuiViewport* viewport = dvz_gui_viewport(gui, figure, NULL);
```

This creates an owned offscreen `DvzAppWindow` for `figure`, connects its live-image sink to ImGui,
and displays it with:

```c
dvz_gui_viewport_window(viewport, "Datoviz viewport", NULL, 0);
```

`dvz_gui_viewport_from_window()` is the advanced path for integrations that already own an
offscreen source app-window.


## Raw ImGui C API

Datoviz also exposes the generated cimgui binding through `datoviz/imgui.h`. This is the raw
version-coupled API with upstream `ig*` names, such as `igBegin()`, `igTextUnformatted()`, and
`igButton()`. It is intended for advanced code that needs Dear ImGui coverage beyond the curated
`dvz_gui_*` helpers.

Raw `ig*` calls are valid inside a Datoviz GUI callback because Datoviz sets the current ImGui
context before invoking user GUI code. The raw layer should not own Datoviz concepts such as
`DvzGuiViewport`; those remain in `datoviz/gui.h`.


## Input

Mouse input can be forwarded from the ImGui image item to the source input router. This is enabled
by default through `DVZ_GUI_VIEWPORT_FLAGS_FORWARD_INPUT`.

Scene controllers should be attached through the viewport input router:

```c
dvz_panel_set_panzoom(panel, dvz_gui_viewport_input(viewport), 0);
```

The viewport forwards pointer press, move, release, and wheel events in source-window coordinates,
including the current keyboard modifier mask. During a button drag, forwarding continues even if
the pointer leaves the ImGui item, and out-of-bounds drag coordinates are kept as raw source-window
coordinates so controllers receive the full drag delta.

Clicking a viewport gives it keyboard focus for Datoviz input routing. GLFW key press, release, and
repeat events are forwarded to that viewport only while no regular ImGui widget wants keyboard
capture. Clicking another ImGui item clears the focused viewport.


## Visibility

Hidden or collapsed ImGui viewport windows stop rendering their source figure after the first
source image is available. This avoids spending GPU time on hidden dock tabs. Set
`DVZ_GUI_VIEWPORT_FLAGS_RENDER_WHEN_HIDDEN` when continuous background rendering is required.


## Ownership

`dvz_gui_viewport()` owns the source app-window render policy and disables rendering when the
viewport is destroyed. The app still owns the underlying app-window storage and releases it during
`dvz_app_destroy()`.

`dvz_gui_viewport_from_window()` borrows the source app-window. The caller keeps ownership of its
lifetime and render policy.
