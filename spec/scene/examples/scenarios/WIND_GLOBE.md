# Wind Globe

> **Example status:** planned showcase
> **Target:** C example
> **Data:** synthetic (procedural wind field on sphere surface)
> **Validation:** screenshot/readback | manual interaction checklist
> **Agent copy-safe:** yes
> **Role:** showcase

This example is designated as one of the primary hero images for the datoviz v0.4 documentation
landing page. It should be visually spectacular: a rotating textured Earth with wind vector arrows
and streamlines rendered on the 3D sphere surface, demonstrating multi-visual composition,
3D geo-scientific data, and arcball interaction in a single scene.


## Summary

A textured Earth sphere rotates under arcball control. Wind vectors are shown as arrows anchored
to the sphere surface at a regular lat/lon grid. Streamlines trace the flow as 3D paths curving
along the surface. The combination communicates immediately: this is scientific data on a 3D
surface, not a globe viewer.

The example extends `showcases/wind_field.c` (2D) and `showcases/textured_planet.c` (textured
sphere) by combining them into a single 3D scene.


## Feature Pressure

- `dvz_mesh()` with equirectangular texture — textured sphere geometry via `dvz_geom_sphere()`
- `dvz_vector()` in 3D — wind arrows anchored to sphere surface positions
- `dvz_path()` in 3D — streamlines as multi-segment curves on sphere surface
- Arcball controller shared across all three visuals in one panel
- Multi-visual composition in a single panel (mesh + vector + path)
- Lat/lon → 3D Cartesian coordinate conversion (CPU-side, inline utility)


## Data And Resources

All data is synthetic and procedural — no external files required.

**Earth texture**: use the same equirectangular Earth texture as `textured_planet.c`. If bundled
in the data cache, load from there. Otherwise generate a simple procedural land/sea colormap as
fallback (no external download required for smoke test).

**Wind field**: generate a synthetic wind field procedurally matching the style of `wind_field.c`:
- A smooth curl-noise or sinusoidal vector field over lat/lon
- Colormap encoding wind speed (magnitude), matching the graphite_cyan palette
- Same field drives both the arrow vectors and the streamline integration

**Streamlines**: integrate the synthetic wind field forward from seed points distributed across
the sphere surface. Each streamline is a sequence of 3D positions on the unit sphere.


## Scene Shape

- Single panel, full window
- Arcball controller (3D rotation, zoom)
- **Mesh visual**: unit sphere, ~512×256 vertices, equirectangular Earth texture
- **Vector visual**: ~800 arrows (approx 40×20 lat/lon grid), colored by wind speed magnitude,
  cyan–mint palette, anchor positions on sphere surface (radius 1.002 to sit above mesh)
- **Path visual**: ~60 streamlines × ~96 points each, colored by speed, same palette,
  radius 1.003 to sit above vectors
- Background: graphite (#1E1E1E)
- Subtle specular highlight on the Earth mesh (Phong, low shininess)
- No axes, no colorbar — clean cinematic look for hero use

Arrow and streamline positions must be offset slightly above the sphere surface (radius > 1.0)
to avoid z-fighting with the mesh.


## Runtime Behavior

- Arcball controller: drag to rotate, scroll to zoom
- No animation by default (static wind field)
- Optional: slow auto-rotation for the hero screenshot (one full rotation over ~30s), toggled
  by a compile-time flag or `--rotate` CLI argument
- Screenshot capture at 1600×900 for hero image use


## Minimal Target

1. Sphere mesh with Earth texture, arcball, dark background — matches `textured_planet.c`
2. Add vector arrows at lat/lon grid points projected onto sphere surface
3. Add streamlines as 3D paths integrated from the same synthetic wind field
4. Tune colors, arrow scale, and streamline density for visual impact
5. Capture hero screenshot at 1600×900

Lat/lon to 3D conversion (inline, no dependency):
```c
// lat, lon in radians; r = sphere radius (1.002 for vectors, 1.003 for paths)
vec3 latlon_to_xyz(float lat, float lon, float r) {
    return (vec3){
        r * cosf(lat) * cosf(lon),
        r * sinf(lat),
        r * cosf(lat) * sinf(lon),
    };
}
```


## Validation

1. **Smoke**: `./build/examples/c/showcases/wind_globe --png` exits 0, produces a PNG
2. **Visual**: Earth texture visible, arrows distributed uniformly over sphere, streamlines
   trace coherent flow patterns, no z-fighting between layers
3. **Interaction**: arcball rotation smooth at 60fps with all three visuals active
4. **Hero screenshot**: 1600×900 PNG, dark background, visually compelling at thumbnail size


## Open Questions

1. Whether the Earth texture is already in the data bundle or needs to be added.
2. Exact arrow scale and streamline density — tune during implementation for visual balance.
3. Whether auto-rotation should be on by default for the hero screenshot or opt-in.
