# Learn Datoviz

Welcome to the Datoviz user guide. This section introduces the main concepts of the library, shows how to create and customize visualizations, and explains advanced usage patterns when you're ready to go deeper.

---

## What you'll learn

This guide is structured into three levels:

### 1. **Essentials**

Start here if you're new to Datoviz:

- 📌 [Quickstart](../quickstart.md) — create your first scatter plot in just a few lines of code
- 📐 [Main Concepts](common.md) — learn how figures, panels, and axes structure your visual scene

### 2. **Visuals**

Each visual primitive has its own page with examples and documentation:

- 🧭 [Overview](visuals.md)
- 🔺 [Basic](../visuals/basic.md) — raw Vulkan primitives (points, line strips, triangles) with uniform color and no custom shaders
- 🟦 [Pixel](../visuals/pixel.md) — individual pixels with position, color, and size (squares)
- ⚪ [Point](../visuals/point.md) — sized, colored antialiased discs without borders
- ✳️ [Marker](../visuals/marker.md) — symbols with optional borders, using built-in shapes, SVGs, or bitmap images
- ➖ [Segment](../visuals/segment.md) — independent stroked line segments with optional caps
- ➰ [Path](../visuals/path.md) — continuous polylines with optional stroke width
- 🖼️ [Image](../visuals/image.md) — RGB or single-channel images with optional colormaps
- 🔤 [Glyph](../visuals/glyph.md) — glyph-based text rendering
- 🧊 [Mesh](../visuals/mesh.md) — 3D triangle meshes with optional lighting (flat, basic, or advanced), textures, contours, or experimental isolines
- 🔮 [Sphere](../visuals/sphere.md) — 3D spheres with customizable lighting and materials
- 🌫️ [Volume](../visuals/volume.md) — basic volume rendering of dense 3D voxel data
- 🪓 [Slice](../visuals/slice.md) — 2D orthogonal slices through 3D volumes

### 3. **Advanced Topics**

Explore these pages to customize behavior or build more complex apps:

- 🖱️ [Interactivity](interactivity.md) — built-in pan, zoom, and arcball camera controls
- 🎮 [Input](input.md) — define keyboard and mouse event callbacks
- ⏱️ [Timers and Frame Events](events.md) — run code every frame or at regular time intervals
- 🧰 [GUI Support (ImGui)](gui.md) — use Datoviz’s built-in ImGui layer for interactive widgets
- 🔧 [Datoviz Rendering Protocol (DRP)](drp.md) — internal low-level rendering architecture
- ⚙️ [Using Datoviz in C](c.md) — native C interface for full performance and control

---

## Where to go next

- Start with the [Quickstart](../quickstart.md) to render your first plot
- Then read [Main Concepts](common.md) to understand how visuals are positioned and transformed
- Or explore the [Gallery](../gallery/index.md) to see what’s possible

Need full technical details? Visit the [API Reference](../reference/api_py.md).
