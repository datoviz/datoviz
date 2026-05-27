# Earth Cubemap

> **Example status:** informative pressure test
> **Target:** Python or C showcase example
> **Data:** bundled/cached cubemap and Earth texture with deterministic generated fallback
> **Validation:** smoke, camera/animation/asset visual checklist

See [../POLICIES.md](../POLICIES.md) for shared example policy.


## Summary

Create an interactive 3D globe in front of a cubemap deep-space background. The v0.4-required slice
is a true retained textured mesh: UV-mapped sphere or sphere-like planet surface, mesh-bound RGBA
texture sampling, orbit camera, and time-based slow spin. Full cubemap/skybox background behavior
is a later extension unless the first textured-mesh proof already exists. The broader goal is to
exercise cubemap creation, upload, sampling, face orientation, camera-relative background behavior,
and a simple interactive 3D scene.


## User-Visible Result

- A window opens directly into a 3D scene.
- A centered textured Earth mesh looks spherical and rotates slowly.
- A starry universe cubemap fills the background and responds to camera rotation without parallax
  when the later cubemap slice is enabled.
- The user can orbit and zoom around the globe with stable interaction.
- The example runs out of the box after assets are bundled, cached, downloaded, or generated.

Suggested scenario/example id: `earth_cubemap`.


## Feature Pressure Points

- Retained textured mesh with UVs and mesh-side sampled texture binding.
- Mesh lighting/material integration for a spherical or sphere-like surface.
- Cubemap resource upload and sampling in the later full example.
- Skybox/environment rendering that ignores camera translation but follows orientation.
- Time-based object animation independent of frame count.
- Orbit/arcball camera defaults and resize stability.


## Required Data And Resources

Cubemap:

```text
posx.png negx.png posy.png negy.png posz.png negz.png
```

Preferred source once available:

```text
datoviz/data/textures/skybox/deep_space/{posx,negx,posy,negy,posz,negz}.png
```

Preferred local cache:

```text
.cache/datoviz/examples/earth_cubemap/deep_space/
```

The cubemap should be visually quiet near face centers, asymmetric enough to reveal face-order or
orientation errors, and suitable for screenshots. If assets are unavailable, generate deterministic
512x512 RGBA8 faces with seed `0xD42024`, dark tinted backgrounds, point stars, a few larger stars,
and soft nebula bands/clusters.

Earth texture:

- at least one color/diffuse map,
- preferably from `datoviz/data` or another clearly licensed Datoviz example asset,
- optional cloud/night/shading maps only if they do not complicate the first slice.


## Minimal Implementation Target

1. Resolve asset cache paths and populate missing cubemap/Earth assets by download or deterministic
   generation.
2. Load the six cubemap faces with correct ordering/orientation.
3. Create one centered textured globe as a retained mesh with positions, normals, UVs, indices, and
   a mesh-bound RGBA texture.
4. Create a skybox, cubemap visual, environment technique, or low-level cube draw that appears
   infinitely distant when the later full example is in scope.
5. Set an orbit camera framing the full globe with surrounding space visible.
6. Update Earth rotation from elapsed time.
7. Start the event loop normally.


## Scene And Runtime Behavior

- Earth is centered near the world origin and is the orbit target.
- Cubemap translation is camera-relative; rotation changes the visible face/region when the
  cubemap slice is enabled.
- Globe renders in front of the background; skybox never occludes it.
- The example should use current v0.4 scene/runtime paths and should not preserve v0.3 API names.
- Startup after cache population should be fast; runtime should remain smooth on a typical desktop
  GPU.


## Validation

Manual checklist:

- first run populates cache or clearly reports unavailable assets;
- second run reuses cache without redundant downloads/generation;
- Earth is visible, spherical, foregrounded, textured through the mesh path, and slowly rotating
  without jitter;
- the cubemap is visible, nonflat, correctly oriented, and not mirrored when enabled;
- camera orbit changes the background while the skybox has no translation parallax when enabled;
- resize does not break rendering;
- depth/draw order never hides the Earth behind the skybox.


## Optional Extensions

- axial tilt, cloud layer, night lights, terminator/shading,
- tiny moon or second object for depth,
- pause/resume or rotation-speed control,
- small title/label overlay,
- capture/video smoke path.
