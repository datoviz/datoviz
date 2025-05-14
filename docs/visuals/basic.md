# Basic Visual

The **basic** visual in Datoviz provides direct access to low-level GPU rendering using core Vulkan primitives. It offers high performance and flexibility while keeping the API simple.

This visual is well suited for rendering large-scale data as points, thin aliased lines, or uniformly colored triangles, offering precise control over topology with minimal overhead.

---

## Overview

- Uses core **Vulkan primitive topologies**: `point`, `line_strip`, `line_list`, `triangle_list`, and `triangle_strip`
- Accepts **3D NDC positions**
- Supports **per-vertex color**
- Supports **uniform size** but not per-vertex size
- Supports a `group` attribute to separate primitives within a single visual

---

## When to use

Use the basic visual when you need:
- Large-scale, uniform visual primitives
- Full control over rendering topology
- Efficient rendering of dense geometry
- Minimal overhead and high performance

---

## Topologies

### Points

Renders one dot per vertex. Useful for unstyled scatter plots or debugging positions.

```python
visual = app.basic('point', position=position, color=color)
````

---

### Line Strip

Connects vertices into a continuous path: `A-B-C-D` becomes a single polyline.

```python
visual = app.basic('line_strip', position=position, color=color)
```

You can also use the `group` attribute to draw multiple line strips in one call:

```python
visual = app.basic('line_strip', position=position, color=color, group=group)
```

The `group` attribute is a 1D array of consecutive integers from `0` to `n_groups - 1`, where each group ID identifies a set of connected vertices. Connections are broken between points with different group IDs.


---

### Line List

Each pair of vertices forms a separate segment: `A-B`, `C-D`, `E-F`, etc.

```python
visual = app.basic('line_list', position=position, color=color)
```

Requires an even number of vertices. `group` can be used to organize or segment the data but does not affect topology.

---

### Triangle List

Each group of 3 vertices forms a triangle: `A-B-C`, `D-E-F`, etc.

```python
visual = app.basic('triangle_list', position=position, color=color)
```

Useful for rendering arbitrary meshes or flat surfaces. No lighting or shading is applied.

---

### Triangle Strip

Renders connected triangles using a strip pattern: `A-B-C`, `B-C-D`, `C-D-E`, etc.

```python
visual = app.basic('triangle_strip', position=position, color=color)
```

More efficient than `triangle_list` when rendering continuous surfaces.

---

## Grouping

The `group` attribute allows you to encode multiple disconnected geometries in one visual.

For example, rendering many time series as separate line strips:

```python
visual = app.basic('line_strip', position=position, color=color, group=group)
```

* Each group must be identified by a **consecutive integer ID**: `0, 0, ..., 1, 1, ..., 2, 2, ...`
* Transitions between groups break the connection in line-based topologies.

This approach is memory-efficient and avoids multiple draw calls.

---

## Summary

The `basic` visual provides direct access to the core rendering capabilities of Datoviz. It's powerful, fast, and flexible — ideal for high-volume data when you don’t need complex styling.

For advanced visuals with lighting, texturing, or custom shapes, see the other visuals.
