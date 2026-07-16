# Multi-Light Klein Bottle Slice

Status: RC2 candidate; not an RC2 blocker.

This slice adds the smallest coherent scene-lighting extension needed to reproduce the Glumpy
checkerboard Klein bottle with three colored lights. It turns the existing future light-object
model into an implementation-ready work packet and uses the Klein bottle as its native/WebGPU
pressure test.

The semantic sources of truth remain
[LIGHTING.md](../semantics/LIGHTING.md) and
[MATERIAL_LIGHTING_API.md](../proposals/active/MATERIAL_LIGHTING_API.md).


## Reference

The visual target is Glumpy's
[`geometry-parametric.py`](https://github.com/glumpy/glumpy/blob/master/examples/geometry-parametric.py)
example:

1. a piecewise parametric Klein-bottle immersion,
2. an indexed UV surface,
3. a repeating black-and-white checkerboard,
4. red, green, and blue point lights,
5. interactive 3D rotation.

Use the reference for behavior and attribution, but write Datoviz-owned geometry, texture, scene,
and shader code rather than copying its implementation.


## Decisions

This slice follows these boundaries:

1. lights are scene-owned reusable objects;
2. panels select their active light set;
3. materials remain visual-owned;
4. light positions and directions use world space;
5. lighting arithmetic uses linear RGB;
6. native Vulkan and WebGPU consume the same scene and DRP2 semantics;
7. the supported light count is fixed and predictable, with at least the eight-light minimum
   required by `LIGHTING.md`;
8. overflow and invalid light descriptors produce diagnostics rather than silent truncation;
9. the current useful default lighting remains available when a panel has no explicit light set;
10. parametric-surface generation remains local to the example in this slice.

The example helper is not public API. Prefer a descriptive name such as
`example_apply_rgb_three_point_lights()` instead of claiming exact Mathematica defaults. The
documentation may describe the preset as Mathematica-like or three-point-inspired.


## Library Scope


### Public Scene API

Before implementation, finalize a descriptor-oriented API for:

1. creating ambient, directional, and point lights in a scene;
2. updating light type-specific position or direction, linear color, intensity, and point-light
   attenuation;
3. destroying a light explicitly while retaining scene-owned cleanup;
4. assigning, replacing, and clearing a bounded light set on a panel;
5. querying or restoring the default panel light set.

The exact names require a public-API review before code starts. Prefer one `DvzLightDesc` creation
path plus focused setters over constructor proliferation. Do not retain `light_direction` as
material-owned state once explicit panel lights become authoritative; migrate current examples and
decide the RC-to-final compatibility treatment explicitly.


### Retained State And Invalidation

Add retained state for:

1. scene-owned light identity and lifetime;
2. validated semantic light descriptors;
3. panel-local ordered light references;
4. a packed panel-light payload or stable shared light-set resource;
5. light and panel dirty state;
6. reverse references sufficient to invalidate every affected panel after a light update or
   destroy.

Reject cross-scene panel/light references. Destroying a referenced light must remove or invalidate
that reference deterministically before the next frame plan; it must never leave a dangling handle.


### Frame Plan, DRP2, And Shader ABI

The scene should lower panel lights into one normalized lighting payload consumed by lit draw
contracts. The implementation must:

1. keep light uploads distinct from per-visual material uploads;
2. use a fixed-capacity GPU representation with an explicit active count;
3. carry light type, world-space position or direction, linear color, intensity, and attenuation;
4. bind the payload through generic lit-visual lowering facts rather than concrete-family checks;
5. share the payload across lit visuals in the same panel when practical;
6. update GLSL and WGSL material evaluation together;
7. add capability diagnostics if a backend cannot honor the declared light set.

Phong and standard materials should sum ambient, diffuse, and specular contributions from the
active lights. A point light with zero attenuation must support the non-decaying behavior used by
the Glumpy reference; positive attenuation enables distance falloff.


### Two-Sided Lighting

A Klein bottle is non-orientable, so a globally continuous outward normal does not exist. The slice
must provide an explicit two-sided surface-lighting mode that:

1. keeps both triangle faces rasterized;
2. orients the fragment normal toward the evaluated side before lighting;
3. is represented as a normalized material or draw fact rather than a Klein-specific shader path;
4. behaves consistently in GLSL and WGSL;
5. leaves existing one-sided material behavior unchanged unless enabled.

Do not solve this by duplicating coincident triangles or by adding a showcase-name check to generic
pipeline code.


### Default Compatibility

Panels without an explicit light set must retain a useful neutral result. During migration:

1. define the default ambient and directional lights explicitly;
2. compare the new default against current material examples and screenshots;
3. migrate `examples/c/features/lighting.c`, `material_mesh.c`, `mesh_texture.c`, and their Python
   mirrors to the final ownership model;
4. remove obsolete material-owned light state only after examples, generated bindings, and public
   docs use panel lights.

This is v0.4 architecture work, not a v0.3 compatibility exercise.


## Shared Example Preset

Add an example-only C helper and a matching Python gallery helper for a reusable RGB three-point
rig. The helper should:

1. create three point lights with red, green, and blue colors;
2. use documented positions derived from the Glumpy composition;
3. use explicit intensity and attenuation values;
4. assign the three lights to one panel;
5. return failure cleanly without leaking partially created state;
6. remain customizable by examples after creation when useful.

Keep this helper beside the existing example camera, material, and style helpers. Do not add a
Mathematica-branded preset to the stable Datoviz public API in this slice.


## Klein Bottle Showcase

Add `showcases/klein_bottle` as a generated-data, portable-scenario example with a manual Python
mirror.


### Geometry

Generate an indexed surface from the Glumpy piecewise Klein-bottle parameterization. The generator
must:

1. use bounded, deterministic U/V segment counts;
2. duplicate the necessary parameter seams for stable UV interpolation;
3. account for the orientation-reversing seam in position and normal evaluation;
4. compute finite normals away from degenerate samples;
5. produce stable triangle winding even though lighting is explicitly two-sided;
6. validate all index and allocation size arithmetic;
7. fit the result into a documented model-space extent.

Keep the generator local to the example. A public generic parametric-surface API is outside this
slice unless a separate review identifies at least one additional concrete consumer.


### Texture

Generate a small RGBA8 checkerboard at runtime. Use UV coordinates or repeated texture cells to
produce the same dense checker pattern without adding a binary asset or changing the `data`
submodule. Texture sampling must use the retained sampled-field and textured-mesh path.


### Presentation

The showcase should include:

1. the shared RGB three-point rig;
2. two-sided lit textured-mesh rendering;
3. a neutral background that preserves the colored-light gradients and black checks;
4. an arcball controller;
5. deterministic preview rotation for gallery animation;
6. bounded `--png`, `--preview`, DVZR, and native live paths supported by the scenario runner.


### Registration

Wire the example through:

1. `examples/c/CMakeLists.txt`;
2. `examples/c/MANIFEST.yaml` with `data.kind: generated`;
3. C and Python showcase documentation;
4. the Python gallery mirror;
5. the WASM scenario table and live-example registry;
6. targeted stream-shape and browser smoke coverage when the multi-light capability is available.

Do not implement separate browser scene logic. If WebGPU multi-light support does not land in the
same checkpoint, mark the example `webgpu-planned` with an explicit capability reason rather than
claiming parity.


## Tests And Validation

Focused tests should fail before implementation and cover:

1. light descriptor validation for type, finite values, intensity, and attenuation;
2. scene ownership, panel binding, light destruction, and cross-scene rejection;
3. the guaranteed light-count boundary and overflow diagnostic;
4. dirty propagation after light and panel-set updates;
5. one shared light payload across multiple lit visuals in a panel;
6. distinct light sets in adjacent panels;
7. directional, point, ambient, colored, attenuated, and zero-attenuation contributions;
8. two-sided normal orientation in native GLSL and WGSL;
9. repeated frame emission without resource or descriptor churn;
10. Klein geometry counts, bounds, finite positions/normals/UVs, and valid indices;
11. deterministic checkerboard generation and texture binding.

Required validation after implementation:

```sh
just build
just test geom
just test scene
just shader-abi-check
just ctypes
just ctypes-check
python3 tools/check_example_manifests.py
just wasm-scene-smoke
just webgpu-browser-smoke
./build/examples/c/showcases/klein_bottle --png
git diff --check
```

Record unsupported environment-dependent native or browser validation honestly; do not weaken the
portable contract to make a smoke pass.


## Non-Goals

This RC2 candidate does not include:

1. shadows;
2. spot lights;
3. image-based lighting;
4. a full PBR rewrite;
5. an unbounded or dynamically sized GPU light list;
6. a new renderer, Vulkan wrapper, or WebGPU-only scene path;
7. a public generic parametric-surface API;
8. binary texture or gallery payload commits in the `data` submodule.


## Acceptance

The slice is complete when:

1. public scene-owned lights and panel-local light sets have reviewed semantics and bindings;
2. current one-light examples retain intentional default appearance after migration;
3. at least eight lights are accepted and overflow is diagnosed;
4. GLSL and WGSL produce the same three-colored-light contract;
5. two-sided lighting correctly supports the Klein surface;
6. the C and Python Klein examples render a checkerboard bottle under the shared RGB rig;
7. the example is registered with honest native/WebGPU status and deterministic capture proof;
8. focused tests, binding checks, shader ABI checks, manifest checks, and `git diff --check` pass.
