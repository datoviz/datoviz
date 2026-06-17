# Scene Model

Datoviz scenes are retained. User code describes the visualization once, updates the retained state
when data or interaction changes, and lets Datoviz derive the next frame from that state.

## Object Hierarchy

The main hierarchy is:

```text
scene
  figure
    panel
      visual
      adornment
      controller binding
```

## Scene and Figure

A scene is the root owner for user-facing visualization objects. It keeps figures, visuals,
controllers, sampled fields, scales, diagnostics, and the state needed to plan frames. Destroy the
runtime app before destroying the scene.

A figure describes a renderable surface at a pixel size. It can contain one full-panel viewport or
several panels arranged into a grid or custom layout. A figure is not itself a backend window; the
same figure concept can be rendered interactively, offscreen, or through an embedded host when that
path is supported.

## Panels and Visuals

A panel is a viewport with a coordinate domain, transforms, optional controller bindings, and a set
of attached visuals and adornments. Multi-panel figures work by giving panels separate viewports
and, when needed, shared controllers or linked domains.

A visual is a homogeneous batch of renderable items: points, markers, paths, segments, images,
meshes, text, glyphs, spheres, and related families. Each visual owns retained CPU-side attribute
data after the corresponding set-data call returns. A visual becomes visible only after it is
attached to a panel.

## Semantic Scene Objects

Adornments and composites, such as axes, ticks, colorbars, scale bars, labels, legends, and
readouts, are semantic scene objects that lower to ordinary visual work. They should explain intent
at the scene level and reuse the same frame-planning path as regular visuals.

Controllers own navigation state. A controller is bound to one or more panels and maps input events
to view changes. Shared controllers are the preferred way to link panels.

## Frame Planning

Frame planning is the bridge out of retained state. When data, transforms, visibility, resources, or
controller state change, the scene marks the affected work dirty and emits a frame artifact with the
setup, update, and draw work needed by the runtime.

See also:

- [Figure, panel, visual model](figure-panel-visual-model.md)
- [Retained resources](retained-resources.md)
- [Performance model](performance-model.md)
