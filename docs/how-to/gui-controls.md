# GUI Controls

Use this page as a bridge to the maintained interaction and embedding guides. GUI controls are not a
first-class How-To nav page in the v0.4 structure.

## Task Workflow

Use Datoviz controllers for viewport interaction. Use GUI controls only for application state such
as toggles, sliders, and mode switches, then update retained scene objects from that state.

## Minimal Call Sequence

```c
/* Host or cimgui control changes application state. */
/* Application updates Datoviz visual data, visibility, or controller state. */
```

## Canonical Examples

- Gallery: [GUI Controls](../examples/gallery/features/feature_gui_controls.md)
- Source: `examples/c/features/gui_controls.c`
- Gallery: [Raw cimgui GUI](../examples/gallery/features/feature_gui_cimgui.md)
- Source: `examples/c/features/gui_cimgui.c`
- Gallery: [GUI Viewport](../examples/gallery/features/feature_gui_viewport.md)
- Source: `examples/c/features/gui_viewport.c`

## Important Details

These examples are native-only in the current manifest. Keep GUI code at the host layer and use the
Datoviz scene API for rendering changes.

## Common Mistakes

- Replacing standard panzoom or 3D controllers with custom GUI event code.
- Letting GUI state and visual attributes drift apart.
- Copying native GUI code into WebGPU browser routes.

## See Also

- [Handle input events](input-events.md)
- [Embed in Qt](embed-in-qt.md)
- [Update visual data](update-visual-data.md)
