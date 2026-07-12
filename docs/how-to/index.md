# How-To guides

Use these guides when you have a specific Datoviz task to complete. If this is your first scene,
follow the short path below before moving to layout, interaction, or rendering options.

## Recommended first path

1. [Create a scene](create-a-scene.md) with a figure and panel.
2. [Add visuals to the panel](add-a-visual.md) and upload their data.
3. [Open an interactive window](create-a-window.md), or
   [render offscreen](render-offscreen.md) for an exact-size image.
4. [Update visual data](update-visual-data.md) without rebuilding the scene.

For a complete first program, start with the [Quickstart](../start/quickstart.md). In Python, use
`import datoviz as dvz` for the normal NumPy-adapted call form. Use
[the exact binding call form](use-raw-ctypes.md) only when you need explicit pointers, counts, or
other C-shaped arguments.

## Choose a guide by outcome

| I want to... | Start here |
| --- | --- |
| choose how to represent my data | [Choose a visual family](choose-a-visual-family.md) |
| place data in the right coordinate space | [Use coordinate systems](coordinate-systems.md) |
| change positions, colors, sizes, or field values | [Update visual data](update-visual-data.md) |
| map scalar data to colors | [Map scalar values with colormaps](use-colormaps.md) |
| display images, label fields, volumes, or textures | [Use sampled fields and textures](use-sampled-fields.md) |
| arrange several views | [Create multiple panels](multiple-panels.md) |
| add axes, colorbars, legends, or labels | [Add axes](axes.md), [add adornments](adornments.md), or [add annotations](add-annotations.md) |
| navigate 2D or 3D data | [Use panzoom](use-panzoom.md) or [use 3D controllers](3d-navigation.md) |
| animate a scene | [Animate a scene](animation.md) |
| save an image or video | [Save screenshots](screenshots.md) or [export videos](video-export.md) |
| use Datoviz from another environment | [Python](use-python.md), [C/C++](c-integration.md), [Qt](embed-in-qt.md), or [the browser](deploy-to-web.md) |
| diagnose a problem | [Debug rendering](debug-rendering.md), [diagnose WebGPU](debug-webgpu.md), or [diagnose platform issues](diagnose-platform.md) |

## Build interaction step by step

Use the smallest interaction layer that satisfies the task:

```text
input event -> rendered query -> application action
                    |                  |
                    +-> pick item -----+-> select or highlight it
                    +-> probe field ---+-> show or store its value
```

1. Start with a built-in [2D](use-panzoom.md) or [3D](3d-navigation.md) controller for ordinary
   navigation.
2. Add [input event handling](input-events.md) only for custom shortcuts or interaction modes.
3. [Pick items](pick-items.md) when you need the rendered item under the pointer, or
   [probe a field](probe-fields.md) when you need a sampled image, label, or volume value.
4. [Select and highlight data](select-items.md) after a successful query, while keeping semantic
   selection state in the application.

## Rendering and output

- [Configure cameras](configure-cameras.md) and
  [use lighting and materials](lighting-and-materials.md) for 3D scenes.
- [Control depth, blending, and transparency](depth-blending.md) when objects overlap or use alpha.
- [Render offscreen](render-offscreen.md) for deterministic output, then
  [save a screenshot](screenshots.md) or [export a video](video-export.md).
- [Profile rendering performance](profile-performance.md) only after a small correct scene works.

The native Vulkan path is the reference path. Browser WebGPU support is experimental and does not
cover every native feature; check the [WebGPU subset](../reference/webgpu-subset.md) and each
example's published status before choosing a browser workflow.
