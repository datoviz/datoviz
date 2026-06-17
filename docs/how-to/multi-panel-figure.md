# Multi-Panel Figure Workflow

This composed workflow is covered by the Layout guides and linked-panel examples.

## Task Workflow

Create a single figure, split it into panels, add visuals per panel, then share controllers only
where views should stay synchronized.

## Minimal Call Sequence

```text
figure -> panel grid -> panel visuals -> optional shared controller -> render
```

## Canonical Examples

- Gallery: [Panel Grid](../examples/gallery/features/feature_panel_grid.md)
- Source: `examples/c/features/panel_grid.c`
- Gallery: [Multiple Panels](../examples/gallery/features/feature_panel_multi.md)
- Source: `examples/c/features/panel_multi.c`
- Gallery: [Linked Panels With Axes](../examples/gallery/showcases/linked_panels_axes_panzoom.md)
- Source: `examples/c/showcases/panel_linked_axes.c`

## Important Details

Panels share a scene and figure. Do not create multiple scenes just to make a subplot layout.

## Common Mistakes

- Copying a showcase when the panel grid example is enough.
- Expecting linked interaction without sharing a controller.
- Adding adornments to the wrong panel.

## See Also

- [Create multiple panels](create-multiple-panels.md)
- [Link panels and controllers](link-panels.md)
- [Add axes](axes.md)
