> **Execution Status**
> - **Status:** `MOSTLY PROMOTED`
> - **Updated on:** `2026-05-20`
> - **Purpose:** preserve external-UI and hosted-backend rationale after promotion into integration
>   and frame-lifecycle specs.

# UI and Backend Integration

This is a promoted proposal record. Active integration rules live in the integration and
frame-lifecycle specs.


## Decision Addressed

External UI frameworks and host toolkits stay outside the scene object model, but scene APIs must be
good mutation targets for real applications. Widgets mutate retained scene state; the scene performs
validation, dirty tracking, and frame planning through the normal pipeline.


## Canonical Specs

Active rules moved to:

1. [`../../integration/EXTERNAL_UI.md`](../../integration/EXTERNAL_UI.md) for external widgets,
   input routing, ImGui integration, offscreen texture binding, and overlay slots.
2. [`../../integration/HOSTED_BACKENDS.md`](../../integration/HOSTED_BACKENDS.md) for host-owned
   loops, external surfaces, Qt/PyQt/IPython/Jupyter direction, and adapter boundaries.
3. [`../../pipeline/FRAME_LIFECYCLE.md`](../../pipeline/FRAME_LIFECYCLE.md) for per-frame overlay
   ordering.
4. [`../../core/RUNTIME_BOUNDARY.md`](../../core/RUNTIME_BOUNDARY.md) for scene/runtime ownership.
5. [`../../integration/HIGH_DPI.md`](../../integration/HIGH_DPI.md) and
   [`../../integration/THREAD_SAFETY.md`](../../integration/THREAD_SAFETY.md) for DPI and threading
   policy.

Repository grounding includes the window/backend hook in
[`../../../../include/datoviz/window/backend.h`](../../../../include/datoviz/window/backend.h),
including `dvz_window_register_qt_backend(...)`.


## Rationale To Preserve

The split remains:

1. UI frameworks own widgets, docking, inspectors, and application-level interaction.
2. Scene owns visualization semantics and retained state.
3. Runtime/window backends own embedding, surfaces, presentation, and host event-loop integration.
4. Public scene APIs should not expose host-specific types or backend-native handles.

Concrete mutation examples that should continue to pressure the API:

1. opacity sliders mutate visual material state,
2. slice-position sliders mutate volume slice state,
3. color-range widgets mutate scene-owned scales,
4. visibility trees mutate retained region visibility,
5. external inspectors observe picking, probe, validation, and diagnostic state.


## Proposal-Owned Backlog

1. Keep ImGui as an external overlay/tool path, not a scene visual family.
2. Keep Qt/PyQt hosting as a runtime/window concern, not a scene concern.
3. Preserve logical offscreen panel or texture handles for host UI display; scene public APIs should
   not expose Vulkan image views or toolkit descriptors.
4. Ensure host UI can consume input before scene controllers and forward unconsumed input to the
   scene router.
5. Keep scene mutation from Python-side or host callbacks on the same retained scene APIs as native
   callbacks.
6. Add hosted-backend examples without baking one toolkit into core scene semantics.


## Non-Goals Still Valid

1. Specifying a full Qt or PyQt backend implementation here.
2. Turning scene into a widget toolkit.
3. Moving ImGui rendering into scene-owned DRP2 semantics.
4. Defining Python bindings in this promoted note.
