# Gaia Galaxy

> **Agent Pickup**
> - **Category:** `astronomy`
> - **Implementation target:** Polished demo concept; implement in stages so the first slice can run with bounded resources.
> - **Data policy:** Public/downloaded assets require cache metadata and an offline fallback or reduced fixture.
> - **Preprocessing:** Usually required; specify source download, conversion, decimation/packing, and generated cache files.
> - **Validation:** Manual visual checklist plus bounded smoke command; add screenshot/readback validation when feasible.

## Summary

Build a polished Python scene example that renders a Gaia-derived Milky Way star sample as an
interactive 3D transparent point-sprite cloud with per-star color, size, arcball/orbit control, and
slow default rotation. The intended data path is a preprocessed Gaia DR3 `.npz` stored in
`datoviz/data`, downloaded into the normal local cache when missing, with a deterministic synthetic
spiral-galaxy fallback if download or validation fails. The first practical slice should load the
bounded dataset or fallback arrays, create one 3D panel through the scene layer, upload positions,
colors, and sizes, and animate the camera or model. Validate with a bounded smoke run, cache/fallback
checks, the visual checklist, and screenshot or readback validation when feasible.


## Goal

Create a polished Python example for the Datoviz v0.4 branch showing an interactive, smoothly animated 3D galaxy-like star field built from real astronomical data.

The example should run out of the box. It should download a small prepared dataset from the `datoviz/data` GitHub repository when the dataset is not already present in the local Datoviz cache.

The exact Datoviz v0.4 Python API is not yet fixed. This document intentionally avoids binding the implementation to exact method names. It describes the required scene, data, visuals, rendering behavior, and acceptance criteria so that an implementation agent can adapt it to the actual v0.4 API.

## Example name

`gaia_galaxy`

Suggested file names:

- Python example: `examples/python/gaia_galaxy.py`
- Dataset in `datoviz/data`: `gaia/gaia_galaxy_100k.npz`
- This specification: `gaia_galaxy.md`

## User-facing description

A smoothly rotating, interactive 3D view of a Milky Way star sample derived from Gaia DR3. Stars are rendered as transparent, glowing point sprites with color inferred from Gaia BP-RP color and brightness inferred from G-band magnitude. The view slowly rotates by default, while the user can interrupt and control the camera with an arcball/orbit controller.

This is primarily a rendering and interaction stress test for Datoviz v0.4: high-density transparent points, per-point attributes, animation, camera control, resource upload, and framegraph support for transparency.

## Data source

### Preferred source: Gaia DR3 Milky Way star sample

Use a preprocessed subset of the Gaia DR3 `gaiadr3.gaia_source` table.

The dataset should not be queried live every time the example runs. Instead:

1. Prepare the dataset once offline from Gaia DR3.
2. Store the compact processed array in `datoviz/data`.
3. The example downloads the `.npz` file from `datoviz/data` if missing.
4. The example keeps a local cached copy under the normal Datoviz user cache directory.

### Caveat

A Gaia-based 3D star cloud is not a physically complete full-galaxy model. Gaia parallaxes are most reliable for comparatively nearby stars, while the visible Milky Way disk, dust lanes, bulge, and spiral arms are only partially represented.

The example should therefore be presented as a **Gaia-derived Milky Way star sample**, not as a scientifically complete galaxy simulation.

## Dataset preparation specification

The downloadable `.npz` dataset should contain only render-ready arrays. It should not require `astropy`, `astroquery`, or Gaia TAP access at runtime.

Recommended dataset size:

- default: 100,000 stars;
- acceptable range: 50,000 to 300,000 stars;
- keep the file small enough for quick download and CI use, ideally under 20 MB compressed.

Recommended Gaia query fields:

```sql
SELECT TOP 300000
    source_id,
    ra,
    dec,
    parallax,
    parallax_over_error,
    phot_g_mean_mag,
    bp_rp,
    pmra,
    pmdec,
    radial_velocity
FROM gaiadr3.gaia_source
WHERE parallax IS NOT NULL
  AND parallax > 0
  AND parallax_over_error > 10
  AND phot_g_mean_mag IS NOT NULL
  AND bp_rp IS NOT NULL
ORDER BY random_index
````

Recommended `.npz` fields:

```text
pos          float32, shape (N, 3), normalized Cartesian positions
color        float32, shape (N, 4), RGBA star color
size         float32, shape (N,), point size
mag_g        float32, shape (N,), Gaia G magnitude, optional metadata/debug
bp_rp        float32, shape (N,), Gaia BP-RP color, optional metadata/debug
source_id    uint64,  shape (N,), optional picking/debug
```

Recommended preprocessing:

1. Convert `ra`, `dec`, and `parallax` to approximate Cartesian coordinates.

2. Use distance in parsecs as `d = 1000 / parallax_mas`.

3. Convert celestial coordinates to Cartesian coordinates:

   ```text
   x = d * cos(dec) * cos(ra)
   y = d * cos(dec) * sin(ra)
   z = d * sin(dec)
   ```

4. Center positions by subtracting the median.

5. Robustly scale coordinates so that the 99th percentile radius maps to roughly 1.0.

6. Clamp extreme outliers.

7. Convert all render arrays to `float32`.

## Runtime data loading

The Python example should:

1. Define a stable dataset URL in the `datoviz/data` GitHub repository.
2. Resolve a local cache path, preferably through an existing Datoviz data helper if available.
3. If the file is missing locally, download it.
4. Load the `.npz` file with NumPy.
5. Validate the expected fields and shapes.
6. Fall back to a deterministic small synthetic spiral galaxy only if download fails, with a clear console warning.

Pseudo-code:

```python
path = datoviz_data_file("gaia/gaia_galaxy_100k.npz")
if not path.exists():
    download_from_datoviz_data("gaia/gaia_galaxy_100k.npz", path)

data = np.load(path)
pos = data["pos"].astype(np.float32)
color = data["color"].astype(np.float32)
size = data["size"].astype(np.float32)
```

## Scene requirements

The example should use the high-level Datoviz v0.4 scene layer, not low-level Vulkan/vklite calls.

Required scene elements:

* one window/canvas, approximately 1280 x 900 by default;
* one full-window 3D panel;
* one 3D camera with perspective projection;
* one transparent point/sprite visual for stars;
* one arcball/orbit interaction controller;
* one continuous animation that slowly rotates the galaxy or the camera;
* optional overlay text showing dataset name, number of stars, and basic controls.

## Visual selection

Preferred visual: transparent point sprite / marker visual.

The visual must support:

* `position`: per-star `vec3` attribute;
* `color`: per-star `vec4` attribute;
* `size`: per-star scalar attribute;
* circular/smooth point sprite rendering;
* alpha blending or weighted blended order-independent transparency;
* optional soft radial falloff in the fragment shader.

If the v0.4 visual library has distinct visual names, choose in this order:

1. `PointSpriteVisual`, `MarkerVisual`, or equivalent GPU point sprite visual;
2. instanced billboard quads facing the camera;
3. plain point-list visual only if sprite rendering is not yet available.

## Rendering style

Target look:

* black or very dark blue background;
* subtle density structure from accumulated transparent stars;
* color variation visible but not garish;
* no axes by default;
* no grid by default;
* minimal overlay text, if any;
* camera placed to see the cloud at an oblique angle.

Suggested default camera:

```text
target = (0, 0, 0)
position = (0.0, -2.6, 1.2)
up = (0, 0, 1)
fov = 45 degrees
near = 0.01
far = 100
```

## Transparency requirements

Preferred implementation:

* mark the star visual as transparent;
* render it through the transparent stage;
* use weighted blended order-independent transparency if available;
* otherwise use standard alpha blending with depth write disabled and depth test enabled or carefully configured.

Expected framegraph behavior:

```text
transparent stars -> OIT accumulation pass -> OIT resolve pass -> main color target
```

If OIT is not yet implemented in the current v0.4 branch, the example should still work with standard alpha blending and contain a small comment noting that OIT should be enabled when available.

## Animation requirements

Default behavior:

* continuous slow rotation around the vertical axis;
* smooth, frame-rate-independent motion based on elapsed time;
* no sudden jumps;
* deterministic initial phase.

The rotation may be implemented in either of two ways:

1. Preferred: animate a model transform on the star visual.
2. Acceptable: animate the camera azimuth around the target.

The user must still be able to interact with the arcball/orbit controller while animation is active.

Recommended interaction policy:

* while the user drags the mouse, temporarily reduce or pause automatic rotation;
* after a short idle delay, resume slow rotation smoothly;
* mouse drag: orbit/arcball;
* mouse wheel or trackpad: zoom;
* right drag or modifier drag: pan, if supported;
* `Space`: pause/resume rotation, if keyboard events are available;
* `R`: reset camera, optional.

## Performance targets

Minimum target:

* 100,000 stars at interactive frame rate on a mid-range discrete GPU;
* smooth animation without CPU-side per-frame reupload of static star buffers;
* per-frame updates limited to camera matrices, transform uniforms, or small parameter buffers.

Preferred target:

* 300,000 stars remains interactive on a high-end GPU;
* no excessive Python per-frame overhead;
* no allocations in the frame callback except trivial temporary values.

Static star positions, colors, and sizes should be uploaded once. Animation should update only a small transform/camera state.

## API-agnostic implementation outline

```python
import numpy as np
import datoviz as dvz

# 1. Load cached Gaia-derived dataset.
pos, color, size = load_gaia_galaxy_dataset()

# 2. Create app/window/scene.
app = dvz.App(...)
scene = app.scene(...)
panel = scene.panel(...)

# 3. Configure a 3D camera.
camera = panel.camera("3d")
camera.look_at(position=(0, -2.6, 1.2), target=(0, 0, 0), up=(0, 0, 1))
camera.perspective(fov=45, near=0.01, far=100)

# 4. Attach orbit/arcball controller.
panel.controller("arcball")

# 5. Create transparent point sprite visual.
stars = scene.visual("points")
stars.channel("position", pos)
stars.channel("color", color)
stars.channel("size", size)
stars.stage("transparent")
stars.material(
    shape="disc",
    falloff="gaussian",
    blending="weighted_oit",
)
panel.add(stars)

# 6. Add animation callback or scheduler item.
def update(dt, t):
    if not user_is_dragging():
        stars.transform.rotation_z(0.03 * t)
    scene.update(dt)

app.on_frame(update)
app.run()
```

This is intentionally pseudo-code. The final code should use the actual v0.4 Python names.

## Required controls

```text
Left drag      orbit / arcball
Wheel          zoom
Space          pause / resume rotation, if key handling is ready
R              reset view, optional
Esc or Q       quit, if supported by examples convention
```

## Optional visual enhancements

1. **Two-layer stars**

   * Render the same points twice:

     * small sharp core with moderate alpha;
     * larger faint halo with low alpha.
   * This produces a glow effect without bloom.

2. **Subtle galactic disk guide**

   * A very faint translucent plane or fog disk can help reveal orientation.
   * It must remain visually secondary to the Gaia points.

3. **Density-adaptive alpha**

   * Lower alpha for dense central regions, higher alpha for sparse outer stars.
   * Avoid saturating the entire image to white.

4. **LOD slider or keyboard shortcuts**

   * Switch between 50k, 100k, and 300k stars if multiple arrays are bundled.

5. **Picking demo**

   * If picking is ready, clicking a bright star could show `source_id`, `G`, and `BP-RP` in the overlay.
   * This is not required for the first version.

## Testing and acceptance criteria

The example is accepted when all of the following are true:

1. Running the Python script from a clean checkout downloads the dataset automatically if it is missing.
2. The example starts without command-line arguments.
3. A 3D star cloud appears quickly after data loading.
4. Stars are colored individually from Gaia-derived color data.
5. Stars are transparent and visually accumulate into a dense, galaxy-like cloud.
6. The scene rotates smoothly by default.
7. Arcball/orbit interaction works on top of the animation.
8. Static star buffers are not reuploaded every frame.
9. The implementation does not depend on a fixed, speculative v0.4 API name.
10. The example remains reasonably short and readable, suitable for the Datoviz examples gallery.

## Headless/screenshot behavior

If Datoviz v0.4 supports capture through an environment variable or example harness, the script should cooperate with it:

* render at least one frame;
* allow the automated screenshot path to capture a visually representative frame;
* optionally advance the animation by a deterministic fixed time before capture.

Suggested screenshot composition:

* oblique camera view;
* dark background;
* galaxy centered;
* visible warm/cool star colors;
* no UI clutter.

## Notes for documentation/gallery metadata

```yaml
title: Gaia Galaxy
tags: [3D, scatter, transparency, animation, astronomy, Gaia]
description: Interactive transparent 3D star cloud from a Gaia DR3-derived Milky Way sample, with smooth rotation and arcball camera control.
```

## References for the implementation agent

* Gaia Archive: [https://gea.esac.esa.int/archive/](https://gea.esac.esa.int/archive/)
* Gaia DR3 overview / archive documentation: [https://gea.esac.esa.int/archive/documentation/GDR3/](https://gea.esac.esa.int/archive/documentation/GDR3/)
* Astroquery Gaia TAP+ access: [https://astroquery.readthedocs.io/en/latest/gaia/gaia.html](https://astroquery.readthedocs.io/en/latest/gaia/gaia.html)
* Gaia DR3 summary paper: [https://arxiv.org/abs/2208.00211](https://arxiv.org/abs/2208.00211)
* Gaia DR3 variable/source classification paper: [https://arxiv.org/abs/2211.17238](https://arxiv.org/abs/2211.17238)

```

::contentReference[oaicite:2]{index=2}
```

[1]: https://astroquery.readthedocs.io/en/latest/gaia/gaia.html "Gaia TAP+ (astroquery.gaia) — astroquery v0.1.dev303+gcffcc6952"
[2]: https://arxiv.org/abs/2208.00211?utm_source=chatgpt.com "Gaia Data Release 3: Summary of the content and survey properties"
