# Earth Cubemap

> **Agent Pickup**
> - **Category:** `geo`
> - **Implementation target:** Geographic or globe/terrain example with a minimal deterministic mode and optional real assets.
> - **Data policy:** Prefer public datasets with cache metadata; include a synthetic fallback for offline development.
> - **Preprocessing:** Required for real datasets; specify download, projection, tiling, simplification, and cache outputs.
> - **Validation:** Smoke command, camera/interaction checklist, and visual checks for projection or coordinate correctness.


## Summary

Create a self-contained Python example for the Datoviz v0.4 branch that demonstrates **cubemap rendering** with an interactive **3D globe** in front of a **universe / deep-space background**.
Use bundled or cached cubemap and Earth texture assets, with deterministic generated placeholders
if assets are missing. The first practical slice is the fake-sphere Earth, skybox, orbit camera,
and slow spin; validation follows a smoke launch plus manual checks for background rotation, globe
animation, interaction, and stable rendering.

The example must open a window and immediately display:

- a cubemap-based skybox or equivalent background showing a visually pleasing universe / star field,
- a centered Earth rendered as a **fake 3D sphere visual** (or the closest v0.4 equivalent inspired by the v0.3 globe example),
- an orbiting camera that the user can rotate interactively,
- a slowly spinning Earth animation,
- a background that behaves correctly under camera rotation so the user clearly sees the cubemap behind the globe.

This example is primarily intended to **exercise the cubemap path in the Datoviz architecture** while also validating a typical interactive 3D scene workflow.

---

## Main goals

1. **Validate cubemap support** end to end.
2. **Validate interactive 3D camera orbiting** in a simple but visually rich scene.
3. **Validate time-based animation** by slowly rotating the Earth.
4. **Validate textured fake-sphere rendering** for a globe-like object.
5. Provide a **beautiful, out-of-the-box example** suitable for demos, screenshots, and regression testing.

---

## Non-goals

This example does **not** need to:

- be a geospatially accurate Earth visualization,
- implement advanced GIS features,
- provide complex UI panels,
- depend on a finalized v0.4 Python API,
- require manual asset downloads by the user.

It should stay simple, robust, and visually attractive.

---

## Example name

Suggested example name:

`earth_cubemap`

The corresponding markdown specification file is:

`earth_cubemap.md`

The corresponding Python example file could later be named something like:

`earth_cubemap.py`

---

## Expected user experience

When the user runs the example:

1. A window opens directly into a 3D scene.
2. A starry universe / nebula-like cubemap fills the background.
3. A textured Earth appears at the center of the scene.
4. The Earth rotates slowly around its own axis.
5. The user can orbit around the Earth using the usual Datoviz camera interaction.
6. As the camera orbits, the cubemap remains in the background and the change in camera orientation is clearly visible in the skybox.
7. The scene should remain smooth and stable without requiring additional setup.

---

## Core scene content

### 1. Background cubemap

The example must include a **cubemap texture** used as a skybox or equivalent infinite background.

Requirements:

- Use six images corresponding to the six cubemap faces, or any cubemap asset format naturally supported by the implementation.
- The content should depict outer space: stars, dark sky, maybe subtle nebulae or galaxies.
- The background should be attractive but not too visually noisy.
- The background must be rendered in a way that makes it appear infinitely far away.
- Camera **rotation** must affect what is seen in the cubemap.
- Camera **translation** should not create parallax in the skybox. If the implementation uses a classic skybox, the translation component of the view transform should effectively be ignored for that object.

Implementation freedom:

- A dedicated skybox primitive, cubemap visual, environment visual, or low-level cube draw is all acceptable.
- The exact shader and object type are not prescribed.

Recommended first environment map:

- Use a deterministic deep-space cubemap: mostly black star field with a subtle blue/violet nebula,
  a faint Milky-Way-like band, and enough asymmetric features to make camera rotation obvious.
- Prefer six square RGBA images with the conventional face roles:
  `posx`, `negx`, `posy`, `negy`, `posz`, `negz`.
- Keep the image content visually quiet near the center of each face so the Earth remains the focus.
- Include a few recognizable off-center star clusters or nebula patches so incorrect face ordering,
  mirroring, or upside-down faces are easy to spot during manual testing.
- If checked-in assets are not available, generate the six faces deterministically as a tiny fixture
  or download them from `datoviz/data` and cache them locally.

Recommended source/generation contract:

- Preferred source, once assets are added to the shared data repository:
  `datoviz/data/textures/skybox/deep_space/{posx,negx,posy,negy,posz,negz}.png`.
- Preferred local cache path for downloaded or generated files:
  `.cache/datoviz/examples/earth_cubemap/deep_space/{posx,negx,posy,negy,posz,negz}.png`.
- If the remote cubemap is unavailable, the example may generate the six PNG faces on first run
  with a fixed seed, for example `seed = 0xD42024`, square size `512 x 512`, and RGBA8 output.
- The generated fallback should combine:
  - a dark radial background with slightly different tint per face,
  - deterministic point stars with a small number of larger bright stars,
  - one or two soft nebula bands or clusters that continue approximately across neighboring faces,
  - subtle face labels or hidden test markers only in debug builds, not in the gallery output.
- Generated assets should be written to the same cache path and reused on later runs.

For this example, "environment map" means the visible cubemap background. It does not need to drive
image-based lighting, reflections, or PBR material response. Those are useful future extensions, but
the first implementation should prove cubemap creation, upload, sampling, face orientation, and
camera-relative skybox behavior.

### 2. Globe / fake sphere visual

The example must include a single Earth at the center of the scene.

Requirements:

- Use a **fake sphere** visual, impostor sphere, textured sphere billboard, or the closest equivalent available in v0.4.
- The rendering should look clearly spherical, not flat.
- The Earth should use a visually pleasing texture map.
- The Earth radius should be large enough to be the obvious focal point of the scene.
- The initial framing should show the full globe with some surrounding space visible.

Strong recommendation:

- Reuse the conceptual approach of the existing v0.3 globe example if helpful, but do not hardcode the spec to the old API.

Optional enhancements, only if easy:

- a mild axial tilt,
- a subtle cloud layer,
- night lights or simple shading,
- a terminator effect or lighting that improves depth perception.

These are optional. The minimal successful version is a textured rotating Earth that reads clearly as a globe.

### 3. Camera interaction

The camera should support an **orbit controller** or equivalent interaction style.

Requirements:

- The user can rotate around the Earth with the mouse.
- Zoom in/out should be supported if naturally available.
- The Earth should stay near the center of interaction.
- The example should start with a good default view.

The main thing to validate is that the user can move the viewpoint around the globe and see the cubemap background respond correctly.

### 4. Earth rotation animation

The Earth must rotate slowly over time.

Requirements:

- Rotation must be time-based, not frame-count-based.
- The speed should be slow and pleasant for visual inspection.
- A full revolution should take long enough to look natural, roughly on the order of tens of seconds to a few minutes.
- The rotation axis should be stable.

A simple constant angular velocity is sufficient.

---

## Data and asset requirements

The example must work **out of the box**.

### Asset sourcing policy

If the required assets are not present locally, the example should automatically download them from the **`datoviz/data` GitHub repository** and cache them locally for reuse.

Requirements:

- No manual download step.
- The first run may download the assets.
- Later runs should reuse the cached copies.
- The cache location should be deterministic and user-friendly.
- If Datoviz already has a standard asset cache helper, use it.
- If not, implement a tiny local helper inside the example or in a shared utility.

### Recommended asset types

The exact file names are not prescribed, but the example will likely need:

1. **Cubemap assets**
   - six universe / skybox images,
   - ordered and mapped consistently to the expected cubemap faces.

2. **Earth texture assets**
   - at minimum one Earth color texture,
   - optionally additional textures if needed by the chosen rendering approach.

### Asset quality expectations

- Use aesthetically pleasing assets that are suitable for demos.
- Prefer assets already available in `datoviz/data`.
- If multiple candidates exist, prefer those that are:
  - visually clean,
  - sufficiently high resolution,
  - clearly licensed or already intended for Datoviz examples,
  - simple to load in Python.

### Robustness

The example should fail gracefully if downloading is impossible.

Recommended behavior:

- print a clear message describing the missing asset,
- indicate where it tried to download from,
- avoid cryptic stack traces when possible.

---

## Rendering and scene behavior requirements

### General visual requirements

- The scene should look polished enough for documentation and promotion.
- The Earth must remain visually distinct from the background.
- The cubemap must not look like a flat backdrop.
- The example should demonstrate true 3D interaction, not a static image.

### Depth and layering

- The globe must render in front of the background.
- The background should behave like a distant environment.
- Depth testing / draw order should be configured so the skybox never incorrectly occludes the globe.

### Transform conventions

- The Earth should be centered near the world origin.
- The orbit controller should use that origin as the natural target.
- The globe spin should be a local object animation.
- The cubemap should track camera orientation appropriately.

### Performance expectations

This is not a stress test, but it should still be lightweight and responsive.

- Startup should be fast after assets are cached.
- Runtime interaction should be smooth on a typical desktop GPU.
- No need for optimization heroics.

---

## Suggested implementation structure

This section is intentionally **API-agnostic**. It describes the conceptual steps, not exact function names.

### 1. Initialize application / canvas / scene

Create the standard Datoviz Python application objects needed to open an interactive 3D window.

Expected components in principle:

- application or event loop object,
- figure / canvas / window,
- a 3D scene or panel,
- a perspective camera,
- an orbit interaction controller.

### 2. Resolve local cache paths

Determine a local cache directory for example assets.

Suggested behavior:

- check whether the cubemap images already exist,
- check whether the Earth texture already exists,
- if not, download them from `datoviz/data`.

The implementation may use:

- a Datoviz helper,
- a small utility function,
- or a generic HTTP download helper.

### 3. Load cubemap assets

Load the six cubemap face images and create a cubemap texture or equivalent environment resource.

Important:

- Ensure correct face ordering.
- Ensure image orientation is correct.
- The final skybox should not appear mirrored or scrambled.

### 4. Create background skybox / cubemap visual

Create the scene object that renders the cubemap as an environment background.

Possible implementation styles:

- dedicated skybox visual,
- cube geometry rendered with a cubemap shader,
- full-screen background technique sampling the cubemap.

Any approach is acceptable if the user perceives a correct 3D cubemap background.

### 5. Load Earth texture(s)

Load the Earth image asset(s) needed by the chosen globe visual.

At minimum:

- one color map / diffuse map.

### 6. Create globe visual

Create a globe at the center of the scene.

Possible implementation styles:

- fake sphere visual,
- impostor sphere visual,
- billboard-based sphere with spherical texturing,
- any close v0.4 equivalent.

The globe should be configured with:

- center position at or near `(0, 0, 0)`,
- an appropriate radius,
- the Earth texture,
- any lighting/shading parameters that improve appearance.

### 7. Set initial camera state

Set a pleasant initial viewpoint.

Suggested default qualities:

- full Earth visible,
- enough margin to see the space background,
- perspective projection,
- near/far settings that are numerically stable.

### 8. Add animation callback

Register a per-frame or timer-driven callback that updates the Earth rotation angle as a function of elapsed time.

Requirements:

- continuous and smooth,
- based on absolute or delta time,
- independent of frame rate.

### 9. Start event loop

Run the application normally.

---

## Behavioral acceptance criteria

An implementation of this spec is considered successful if all the following are true.

### Visual acceptance

- A universe-like cubemap background is visible.
- A globe resembling Earth is visible in the foreground.
- The Earth looks spherical.
- The Earth rotates slowly over time.
- The background remains behind the Earth.

### Interaction acceptance

- The user can orbit the camera around the Earth.
- Rotating the camera changes the visible region of the cubemap.
- The skybox does not translate like a nearby object.
- Interaction feels stable and natural.

### Asset acceptance

- The example runs without requiring manual asset placement.
- Missing data is downloaded automatically from `datoviz/data`.
- Downloaded data is cached and reused.

### Architectural acceptance

- The example genuinely exercises the cubemap path.
- The example uses the Datoviz v0.4 architecture and Python bindings as they exist at implementation time.
- The spec is implemented without coupling the design to obsolete v0.3 function names.

---

## Recommended manual test checklist

A developer implementing this example should verify the following manually:

1. **First run without cache**
   - assets download correctly,
   - scene opens successfully.

2. **Second run with cache**
   - no redundant downloads,
   - startup is faster.

3. **Camera orbit**
   - drag interaction rotates around the globe,
   - star background changes appropriately.

4. **Earth animation**
   - Earth rotates continuously,
   - no jitter,
   - speed remains reasonable.

5. **Background correctness**
   - no obvious face seam mistakes,
   - no upside-down face issue,
   - no incorrect left/right mapping.

6. **Depth correctness**
   - Earth is never hidden by the skybox.

7. **Resize behavior**
   - window resize does not break rendering.

---

## Possible stretch goals

These are optional and should only be implemented if they are easy and do not complicate the example.

- add a tiny moon or a second object for more depth,
- add stars or labels as an overlay,
- add a keyboard shortcut to pause/resume Earth rotation,
- add an on-screen title or small help text,
- expose a simple speed parameter.

These must remain secondary to the main goal of testing cubemap support.

---

## Notes for the implementing agent

- Keep the code short and readable.
- Prefer the highest-level v0.4 scene API that cleanly supports this example.
- If a direct cubemap abstraction does not yet exist, use the lowest-level route necessary, but keep the example user-facing and simple.
- If the exact fake-sphere visual has changed in v0.4, use the closest equivalent that still delivers a convincing textured globe.
- If the scene API differs from v0.3, preserve the **intent** of the example rather than the old API shape.
- If multiple data asset choices exist in `datoviz/data`, choose the simplest robust set and document them in the example source comments.

---

## Deliverable expectations

The final Python example should:

- be runnable as a normal Datoviz example,
- contain any tiny asset-cache helper needed for out-of-the-box operation,
- clearly demonstrate cubemap rendering,
- be suitable for inclusion in the gallery or showcase examples.
