# Embed in Qt

Integrate Datoviz with a Qt application that owns the window and event loop.

Qt hosting is implemented as an advanced integration through an optional native bridge. Vulkan-enabled Qt and PyQt are now published on conda-forge; Datoviz's split conda provider has development-package proof on Linux and Apple Silicon macOS. Official Datoviz provider packages remain unpublished, with RC3 inclusion under evaluation and RC4 as the fallback.

!!! info "At a glance"

    - **Status:** Implemented from source; development conda provider validated on Linux and Apple Silicon macOS. RC3 inclusion is provisional.
    - **Languages:** C++/Qt and Python/PyQt6 through the optional bridge.
    - **Prerequisites:** Matching Qt development/runtime libraries, Vulkan hosting support, and a built/discoverable bridge.
    - **Result:** Qt owns the window/event loop while Datoviz renders one hosted view and receives forwarded events.

## Task workflow

Let Qt own the application shell, menus, widgets, and native window. Let Datoviz render the figure
inside the hosted view. Use the external-surface or viewport integration path closest to the
platform you target.

## Minimal workflow

1. Let the host toolkit create or expose the native surface.
2. Create Datoviz with any required host Vulkan instance extensions.
3. Attach a Datoviz view to the external surface or hosted viewport path.
4. Let the host event loop drive resize, input, frame requests, and `dvz_view_render_once()`.

The host remains responsible for the window and events. Datoviz borrows the surface, updates its
retained scene from forwarded input, renders one frame, and asks Qt to schedule the next update.

<div class="dvz-sequence" role="list" aria-label="Qt and Datoviz event flow">
  <div class="dvz-sequence__step" role="listitem">
    <span class="dvz-sequence__number">Step 1</span>
    <strong>Qt host</strong>
    <span>Owns the window and event loop.</span>
  </div>
  <div class="dvz-sequence__step" role="listitem">
    <span class="dvz-sequence__number">Step 2</span>
    <strong>Forward events</strong>
    <span>Send resize and input to the view.</span>
  </div>
  <div class="dvz-sequence__step" role="listitem">
    <span class="dvz-sequence__number">Step 3</span>
    <strong>Render once</strong>
    <span>Datoviz draws to the Qt surface.</span>
  </div>
  <div class="dvz-sequence__step" role="listitem">
    <span class="dvz-sequence__number">Step 4</span>
    <strong>Request update</strong>
    <span>Qt schedules the next frame.</span>
  </div>
</div>

Use the GLFW external-surface example as the closest maintained native embedding reference.

This page describes an ownership protocol, not a copy-pasteable single function. Start from the
linked Qt example because instance-extension discovery, surface creation, event forwarding, and
teardown must agree with the host platform.


## Native Qt examples

The native Qt examples are built only when Qt development packages are available. Enable the
optional bridge explicitly when validating the provider:

```sh
DVZ_CMAKE_ARGS="-DDVZ_ENABLE_QT_BRIDGE=ON" just build
./build/examples/qt/hosted_qt_smoke 120
./build/examples/qt/qt_hosting --smoke-ms 1000
```

`hosted_qt_smoke` is the minimal contract smoke. `qt_hosting` embeds the hosted Datoviz
window in a Qt Widgets layout and lets widget callbacks mutate retained scene data.

<figure class="dvz-output-example">
  <a href="../../examples/advanced/qt-hosting/">
    <img src="../../assets/gallery/v0.4/advanced/advanced_qt_hosting.webp"
         alt="A Datoviz scene embedded beside controls in a Qt Widgets application" loading="lazy">
  </a>
  <figcaption>
    <strong>Qt-hosted view.</strong> Qt owns the application shell and controls while Datoviz renders
    the scene inside the hosted surface.
    <a href="../../examples/advanced/qt-hosting/">Open the Qt example</a>.
  </figcaption>
</figure>


## Ownership contract

| Owner | Responsibilities |
| --- | --- |
| Qt | Native window, event loop, widget state, and repaint scheduling. |
| Datoviz | Graphics context, scene state, hosted view, and rendering. |

Qt adopts the Datoviz-created Vulkan instance with `QVulkanInstance::setVkInstance()`. Qt does not
own that instance; Datoviz destroys it when `dvz_app_destroy()` runs.

The Qt surface is borrowed by Datoviz. Qt remains responsible for creating and destroying the
surface; Datoviz renders into it while it is available.


## Event forwarding

Forward host events through the public hosted app API:

| Host event | Datoviz call |
| --- | --- |
| resize | `dvz_view_emit_resize()` |
| mouse move, press, release | `dvz_view_emit_pointer()` |
| wheel | `dvz_view_emit_wheel()` |
| key press, release, repeat | `dvz_view_emit_key()` |
| committed UTF-8 text on press or repeat | `dvz_view_emit_text()` |

Physical keys must come from a tested native scan-code mapping; when Qt cannot provide physical identity, emit `DVZ_KEY_UNKNOWN` instead of substituting `QKeyEvent::key()`. Route nonempty `QKeyEvent::text()` separately on press and repeat, with no text event on release.

Qt should schedule rendering with `QWindow::requestUpdate()`, then call `dvz_view_render_once()` from the update path. Widget callbacks should mutate retained scene state, request a frame, and let Datoviz render through the existing view.


## Surface teardown

Before Qt destroys or recreates the native surface, release Datoviz's presentation resources:

1. Clear the request-frame callback with `dvz_view_set_request_frame_callback(view, NULL, NULL)`.
2. Update the hosted view with a null surface tuple.
3. Call `dvz_view_render_once()` once so Datoviz observes the unavailable surface and releases the
   present swapchain.

The Qt adapter handles `QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed`. A warning that the
surface is unavailable during this cleanup pass is expected; graphics validation errors are not
expected.


## PyQt hosting

The Python hosted path uses the optional `datoviz_qtbridge` provider. RC2 wheels contain the
`datoviz.qt` adapter but not the native bridge. The `datoviz[qt]` extra installs PyQt6 only; it does
not make Qt hosting usable by itself. For RC2, build the bridge with `DVZ_ENABLE_QT_BRIDGE=ON` or
supply a compatible source-built bridge separately. The loader checks the package directory, its
`.libs` directory, common source-checkout build directories, and the platform library search path.
Set `DATOVIZ_QTBRIDGE_LIBRARY` only when discovery needs an explicit bridge path.

For an installed package or a bridge discoverable through the platform library path, run:

```sh
python -m datoviz.qt
```

For an isolated provider smoke with PyQt6, use:

```sh
uv run --isolated --with PyQt6 python -m datoviz.qt
```

Keep failure diagnostics explicit: a missing bridge library, unsupported PyQt/PySide binding,
missing `QVulkanInstance` features, or Qt runtime mismatch should fail before rendering starts.

For a source-checkout bridge that is not otherwise discoverable, pass its exact path:

```sh
DATOVIZ_QTBRIDGE_LIBRARY=build/qtbridge/libdatoviz_qtbridge.so \
python examples/python/qt/hosted_pyqt.py --smoke-ms 1000
```

The usual source-build filenames are:

| Platform | Bridge path or filename |
| --- | --- |
| Linux | `build/qtbridge/libdatoviz_qtbridge.so` |
| macOS | `build/qtbridge/libdatoviz_qtbridge.dylib` |
| Windows | `build/qtbridge/datoviz_qtbridge.dll` or `build/qtbridge/libdatoviz_qtbridge.dll` |

Prefer normal package or platform-library discovery and verify it with `python -m datoviz.qt`.
Use `DATOVIZ_QTBRIDGE_LIBRARY` only when the bridge is outside those discovery locations.

System PyQt packages may lack the required Vulkan instance API even when PyQt imports successfully.
Treat that as an environment limitation, not as a Datoviz rendering failure.

## Development conda provider

Use this route when you have an indexed channel containing locally built `libdatoviz`, `datoviz`, and `datoviz-qtbridge` packages for your platform. The channel must contain all three outputs from the same source build; conda-forge supplies their runtime dependencies. This is development-package guidance, not an installation command for a published RC3 provider. The validated development packages retain `0.4.0rc2` metadata and are distinct from the published RC2 wheels.

Replace `path/to/local-channel` with your indexed channel directory or URL. Create a fresh environment with the validated Python 3.11 runtime and exact Qt/PyQt builds:

```sh
conda create --name datoviz-qt --override-channels --strict-channel-priority -c path/to/local-channel -c conda-forge python=3.11 datoviz-qtbridge=0.4.0rc2 "qt6-main=6.11.1=*_2" "pyqt6=6.11.0=*_5"
conda activate datoviz-qt
python -m datoviz.qt
```

The provider requires the matching Datoviz Python and native packages, so the solver installs all three outputs. Keep Qt, PyQt, the bridge, and the Vulkan loader in the same managed environment. On macOS, the base native package also supplies conda MoltenVK; that runtime declares macOS 14 as its minimum. The macOS 15 wheel floor is a separate packaging contract. Do not copy the standalone wheel's private Vulkan/MoltenVK payload into this environment or replace the conda Datoviz packages with a pip wheel.

The probe reports the bridge path, bridge ABI, bridge Qt version, PyQt Qt version, and required Vulkan binding methods. Expect ABI 1 and matching Qt 6.11.1 versions for this package set. A successful probe checks provider discovery and compatibility; it does not create a window or prove rendering. Continue with the [PyQt hosted example](../examples/advanced/qt-hosting.md) to exercise rendering, resize, interaction, and close/reopen on a working native desktop.

| Platform | Development evidence | Remaining limitation |
| --- | --- | --- |
| Linux x86_64 | Split-package tests, base rendering without Qt, and hosted resize/rendering with Vulkan validation pass under Xvfb/Mesa llvmpipe. | Hardware interaction and exact RC3 artifacts remain separate checks. |
| macOS arm64 | Split-package tests, base rendering, Cocoa surface creation, managed loader/MoltenVK identity, and hosted resize/rendering pass. | Intel and minimum-macOS runtime are untested; attended interaction and teardown allocation behavior remain pending. |
| Windows x86_64 | All split outputs compile and package; native package file-layout tests pass. | Python import is blocked by Smart App Control rejecting the unsigned GLFW dependency; provider rendering on AMD/NVIDIA remains untested. |

If Windows reports `WinError 4551`, inspect the Code Integrity events to identify the rejected dependency. The recorded development run was blocked while loading conda's `glfw3.dll`, before Qt rendering started. [Microsoft documents no per-app Smart App Control exception](https://support.microsoft.com/en-us/windows/security/threat-malware-protection/smart-app-control-frequently-asked-questions). Use a trusted dependency distribution or a suitable validation host; changing host security policy requires a separate decision. A dependency solve, successful import, or green source-build CI does not substitute for installed-provider rendering proof.


## Important details

Qt embedding is a host-integration task. Do not create a second rendering stack; adapt the existing
Datoviz hosted view path.

Qt and Datoviz must agree on Vulkan instance extensions before the Datoviz app is created. Query
Qt's required extensions first, then pass them into Datoviz app creation.

Destroy Datoviz presentation/app state while the borrowed Qt surface and adopted instance context
are still valid. Then allow Qt to finish destroying its surface/window objects.

## Common mistakes

- Letting both Qt and Datoviz own the same native graphics handle.
- Handling resize in the UI but not notifying the Datoviz view.
- Copying Datoviz implementation code instead of using public integration surfaces.
- Destroying or recreating the Qt surface without giving Datoviz a cleanup render pass.
- Rendering directly from arbitrary widget callbacks instead of scheduling a frame.
- Treating PyQt import success as proof that the required Vulkan hosting API is available.

## See also

- [Open an interactive window](create-a-window.md)
- [Handle input events](input-events.md)
- [Diagnose build and platform issues](diagnose-platform.md)

??? example "Complete and related examples"

    - Canonical complete example: [Qt Hosting](../examples/advanced/qt-hosting.md) - Source: `examples/qt/qt_hosting.cpp`
    - [External Surface GLFW](../examples/advanced/external-surface-glfw.md) - Source: `examples/c/advanced/external_surface_glfw.c`
    - [GUI Viewport](../examples/features/gui-viewport.md) - Source: `examples/c/features/gui_viewport.c`
