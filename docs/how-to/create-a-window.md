# Open an Interactive Window

Run a scene in a native window for desktop interaction.

You need a scene with a figure and panel. The result is a window whose event loop renders the
figure and forwards resize and input events to its controllers.

## Use this when

- You want a visible native window with mouse, keyboard, resize, and controller interaction.
- You are running on a desktop environment with GLFW and a Vulkan-capable GPU.
- You need to test interactive behavior rather than only render one image.
- You want the direct Datoviz app path, not a Qt-hosted or browser WebGPU view.

Use [offscreen rendering](render-offscreen.md) for headless CI, deterministic screenshots, and
server-side image generation. Use [Qt embedding](embed-in-qt.md) when another UI toolkit owns the
native window.

## Minimal sequence

Build the retained scene first: create the scene, figure, panel, visuals, data, and panel
attachments. Then create the native app and attach a window view to the figure.

```c
DvzApp* app = dvz_app(scene);
if (app == NULL)
    return -1;
DvzView* view = dvz_view_window(app, figure, width, height, "Datoviz");
if (view == NULL)
{
    dvz_app_destroy(app);
    return -1;
}
DvzPanzoom* panzoom = dvz_view_panzoom(view, panel, NULL);
if (panzoom == NULL)
{
    dvz_app_destroy(app);
    return -1;
}

dvz_app_run(app, 0);
dvz_app_destroy(app);
```

Pass `0` to `dvz_app_run()` to run until the window closes. Pass a positive frame count for smoke
tests or deterministic captures.

## Lifecycle

| Object | Created by | Role | Destroy rule |
| --- | --- | --- | --- |
| `DvzScene` | `dvz_scene()` | Owns retained figures, panels, visuals, data, and scene state. | Destroy after the app. |
| `DvzFigure` | `dvz_figure()` | Describes the rendered figure attached to a view. | Owned by the scene. |
| `DvzApp` | `dvz_app(scene)` | Owns the live runtime, GPU context, scheduler, and views. | Destroy explicitly with `dvz_app_destroy()`. |
| `DvzView` | `dvz_view_window()` | Native visible window for one figure. | Owned by the app. |

The usual order is:

```text
scene -> figure/panel/visuals -> app -> view -> controller/input -> run -> destroy app -> destroy scene
```

## Controllers and input

Bind controllers before entering the run loop. For the common panzoom case, create it from the
view so the controller is connected to the live input path.

```c
dvz_view_panzoom(view, panel, NULL);
```

Use 3D controller helpers for 3D panels, such as arcball, turntable, or fly. Input callbacks and
GUI overlays should also be registered before `dvz_app_run()`.

## Backend status

`dvz_view_window()` is a supported native presentation path. It opens a desktop window through the
configured native backend and is not part of the WebGPU/WASM browser path. In the generated WebGPU
example matrix, the canonical app example is therefore classified as `native-only` with the
`native-view` requirement; that matrix label describes browser portability, not the native API's
release status.

`dvz_view_window()` can return `NULL` when GLFW is unavailable, no display is available, window
creation fails, or the GPU/runtime cannot be initialized. In automated environments, prefer
`dvz_view_offscreen()`.

## Complete examples

- [GLFW App](../examples/gallery/runtime/runtime_app_glfw.md) - direct GLFW app lifecycle without
  the scenario runner. Source: `examples/c/runtime/app_glfw.c`.
- [Panzoom](../examples/gallery/features/features_panzoom.md) - 2D controller interaction. Source:
  `examples/c/features/panzoom.c`.
- [Input Events](../examples/gallery/features/features_input_events.md) - native input callback
  handling. Source: `examples/c/features/input_events.c`.

## Important details

The app/runtime is not a scene object. It should be destroyed explicitly with `dvz_app_destroy()`
before `dvz_scene_destroy()`.

One app may drive multiple views, but each view presents one figure. Keep view-specific controllers,
GUI overlays, and callbacks attached to the view or panel they control.

Finite frame counts are useful for smoke tests. Interactive applications normally pass `0` and let
the app scheduler render until the user closes the window.

## Common mistakes

- Creating the app before the scene has a figure and attached visuals.
- Forgetting to check whether `dvz_app()` or `dvz_view_window()` returned `NULL`.
- Binding controllers after `dvz_app_run()`, when the interactive loop is already active.
- Capturing a PNG from a native window view before the first frame has rendered.
- Running a native window path in headless CI instead of using `dvz_view_offscreen()`.
- Sharing platform window handles without using the external-surface integration path.

## Next steps

- [Create a scene](create-a-scene.md)
- [Use panzoom](use-panzoom.md)
- [Handle input events](input-events.md)
- [Render offscreen](render-offscreen.md)
- [Embed in Qt](embed-in-qt.md)
- [App, window, and I/O API reference](../reference/c-api/app.md)
