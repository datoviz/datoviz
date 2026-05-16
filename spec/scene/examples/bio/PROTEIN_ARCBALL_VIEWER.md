# Protein Arcball Viewer

> **Agent Pickup**
> - **Category:** `bio`
> - **Implementation target:** Scientific domain example with staged implementation, starting from deterministic or prepared data.
> - **Data policy:** Synthetic or prepared-cache first slice; real public data may be a second stage with license notes.
> - **Preprocessing:** Document any data conversion script, output schema, coordinate normalization, and cache validation.
> - **Validation:** Smoke run plus domain-specific visual, picking/probe, and performance acceptance criteria.

## Architecture Summary

Build a Datoviz v0.4 Python protein viewer with a 3D arcball camera, ImGui controls, molecular
rendering modes, and SSAO integrated as an early multi-pass scene requirement. Runtime should use
precomputed protein geometry from a compact local cache, downloading curated bundles from
`datoviz/data` when missing, rather than parsing raw PDB or mmCIF files or generating surfaces
interactively. The first practical slice should load a small molecule such as `1CRN` or `1UBQ`,
render one chosen mode with element or chain coloring, provide arcball navigation and basic controls,
and keep the framegraph ready for SSAO/offscreen resources. Validate with a smoke run, molecular
visual checks, picking or probe checks when available, and the staged performance criteria.


## Purpose

Specify a **Datoviz v0.4 Python example** showing an interactive 3D protein viewer with an **arcball camera**, **ImGui controls**, several **molecular rendering modes**, and an **early SSAO integration**.

This document is intentionally **API-agnostic**. It should not assume the final exact Datoviz v0.4 Python API, but it should be concrete enough that an implementation agent can map it onto the actual scene API once available.

The example should be conceptually close to Datoviz v0.3 and to the new v0.4 scene architecture, but it must not rely on unstable function names.

---

## Why this example exists

This example is not just a nice demo. It is meant to **pressure the Datoviz v0.4 architecture**, especially the **scene API**, by requiring:

- a 3D panel with an arcball controller,
- multiple visuals in one panel,
- switching between different geometry representations,
- precomputed geometry loaded from cache,
- offscreen intermediate render targets,
- a multi-pass framegraph,
- **SSAO early in the design**, not as an afterthought,
- post-processing composition,
- transparent/opaque stage handling,
- runtime control via ImGui.

The main architectural goal is to force the scene layer to support a rendering workflow of the form:

```text
molecular geometry -> offscreen buffers -> SSAO -> composite -> final panel output
```

In other words, this example should help validate that the scene API can express:

- render passes,
- compute or fullscreen postprocess passes,
- per-panel offscreen resources,
- pass dependencies,
- pass-local and panel-global resources,
- post-processing that remains easy to use from Python.

---

## Example name

**Example name:** `PROTEIN_ARCBALL_VIEWER`

**Suggested Python file:**

```text
examples/python/protein_arcball_viewer.py
```

**Suggested spec filename:**

```text
PROTEIN_ARCBALL_VIEWER.md
```

---

## High-level user experience

The example opens a single window with one main 3D panel showing a protein. The user can:

- rotate with an arcball/orbit interaction,
- zoom with the mouse wheel,
- pan,
- choose the molecule,
- choose the rendering mode,
- choose the coloring mode,
- enable/disable SSAO,
- adjust SSAO parameters,
- reset the camera,
- optionally enable autorotation.

The initial default should be visually attractive and should demonstrate the scene architecture immediately.

Recommended default:

- molecule: `1UBQ` or `1CRN`
- mode: `surface` or `ball-and-stick`
- coloring: `chain` or `element`
- SSAO: **enabled by default**
- dark background
- perspective camera

---

## Core requirement: SSAO must be integrated early

### Rationale

SSAO is required **early** because it pressures the scene API to support a realistic modern rendering pipeline rather than only direct draw-to-screen workflows.

This example should therefore be designed from the beginning around a **multi-pass scene/framegraph architecture**.

### Consequences for the scene API

The scene API should be able to express, directly or indirectly:

1. **Offscreen color target(s)**
2. **Offscreen normal target**
3. **Offscreen depth target**
4. **A fullscreen SSAO pass** reading depth + normal
5. **An optional blur pass** for SSAO
6. **A composite pass** mixing lit color and AO
7. **Per-panel pass dependencies**
8. **Different visuals assigned to different passes**
9. **Postprocessing controls exposed to Python code**

This is one of the main reasons this example exists.

---

## Required molecular rendering modes

The example must support at least the following rendering modes:

1. **Atoms / space-filling**
2. **Ball-and-stick**
3. **Surface**
4. **Ribbon / cartoon**

Optional extra mode:

5. **Backbone trace**

The rendering modes should ideally be implemented as different visuals or visual groups whose visibility can be toggled, rather than as a single monolithic renderer.

Switching mode should generally **not recompute geometry at runtime**.

---

## Required ImGui controls

At minimum, the GUI should expose:

```text
Molecule:        [1CRN / 1UBQ / 4HHB / 6M0J / ...]
Rendering mode:  [Atoms / Ball-stick / Surface / Ribbon / Backbone]
Coloring:        [Element / Chain / Residue / Secondary structure / B-factor]
SSAO:            checkbox
SSAO radius:     slider
SSAO bias:       slider
SSAO intensity:  slider
SSAO samples:    [8 / 16 / 32]
SSAO blur:       checkbox
Surface opacity: slider
Atom scale:      slider
Bond radius:     slider
Ribbon radius:   slider
Auto rotate:     checkbox
Reset camera:    button
Show axes:       checkbox
```

The exact widget names may vary, but the functionality should be equivalent.

---

## Runtime behavior and caching

The example must work out of the box.

### Data strategy

Runtime should **not** parse raw PDB/mmCIF files or run expensive surface/ribbon generation.

Instead:

- geometry is **precomputed offline**,
- stored in a compact Datoviz-friendly format,
- cached locally,
- downloaded from the `datoviz/data` GitHub repository if not already present.

### Cache behavior

Recommended cache directory:

```text
~/.cache/datoviz/proteins/
```

Recommended data layout:

```text
~/.cache/datoviz/proteins/1ubq/protein.npz
~/.cache/datoviz/proteins/1ubq/metadata.json
```

If the selected molecule is missing locally, the example should:

1. build the expected remote URL,
2. download the bundle,
3. store it in the cache,
4. load it immediately.

The example should not re-download an existing valid cache entry.

---

## Recommended molecules

Use a small curated set of proteins chosen to cover several use cases.

Suggested molecules:

| ID | Purpose | Notes |
|---|---|---|
| `1CRN` | tiny protein | very fast, good sanity check |
| `1UBQ` | small protein | excellent default |
| `4HHB` | multichain protein | useful for chain coloring |
| `6M0J` or equivalent | larger structure/domain | useful for surface + ribbon stress |
| optional ligand-rich example | hetero atoms | useful for ball-and-stick |

The exact set may be adjusted depending on preprocessing cost and bundle size.

---

## File formats for precomputed data

### Recommended format

Use a single compressed `.npz` file plus a small metadata JSON file.

For example:

```text
data/proteins/1ubq/protein.npz
data/proteins/1ubq/metadata.json
```

This is simple, portable, NumPy-friendly, and easy to load from Python.

If later needed, a custom binary format may be introduced, but `.npz` is preferred for the first version.

---

## Precomputed data model

The whole point of preprocessing is to convert structural biology data into arrays that match what Datoviz wants at runtime: flat numerical buffers and index buffers.

The data bundle should therefore be organized around **visual-ready arrays**.

---

## Common metadata

Every molecule bundle should include:

```text
center                 float32[3]
radius                 float32
bbox_min               float32[3]
bbox_max               float32[3]
atom_count             uint32
bond_count             uint32
chain_count            uint32
has_surface            bool
has_ribbon             bool
```

Optional metadata:

```text
pdb_id
name
chains
residue_count
surface_vertex_count
surface_triangle_count
ribbon_vertex_count
ribbon_triangle_count
```

The runtime should recenter the molecule using `center`, and use `radius` to initialize a good camera framing.

---

## Atoms / space-filling representation

### Required arrays

```text
atom_position          float32[n_atoms, 3]
atom_radius_vdw        float32[n_atoms]
atom_radius_ball       float32[n_atoms]
atom_color_element     float32[n_atoms, 4]
atom_color_chain       float32[n_atoms, 4]
atom_color_residue     float32[n_atoms, 4]
atom_color_bfactor     float32[n_atoms, 4]
atom_element           uint8[n_atoms]
atom_chain             uint32[n_atoms]
atom_residue           uint32[n_atoms]
```

### Runtime rendering idea

Preferred:

- instanced sphere mesh, or
- sphere impostor visual.

Each atom is one instance with at least:

- position,
- radius,
- color.

The implementation may use:

- true sphere mesh instancing,
- screen-aligned impostors,
- or any Datoviz-native sphere/glyph visual if available.

---

## Ball-and-stick representation

### Required arrays

```text
bond_index             uint32[n_bonds, 2]
bond_position_a        float32[n_bonds, 3]
bond_position_b        float32[n_bonds, 3]
bond_center            float32[n_bonds, 3]
bond_orientation       float32[n_bonds, 4]
bond_length_radius     float32[n_bonds, 2]
bond_color_element     float32[n_bonds, 4]
bond_color_chain       float32[n_bonds, 4]
```

### Runtime rendering idea

Atoms:

- rendered as spheres with smaller radii than space-filling mode.

Bonds:

- rendered as instanced cylinders, capsules, or a line-like tube visual.

If the implementation supports instanced cylinder meshes, a good instance parameterization is:

- center,
- orientation quaternion,
- length,
- radius,
- color.

---

## Surface representation

The surface must be precomputed offline.

### Required arrays

```text
surface_position        float32[n_surface_vertices, 3]
surface_normal          float32[n_surface_vertices, 3]
surface_color_element   float32[n_surface_vertices, 4]
surface_color_chain     float32[n_surface_vertices, 4]
surface_color_residue   float32[n_surface_vertices, 4]
surface_index           uint32[n_surface_indices]
```

### Runtime rendering idea

Use one indexed triangle mesh visual with:

- vertex positions,
- vertex normals,
- per-vertex color,
- triangle indices.

Surface opacity should be controllable from the GUI.

---

## Ribbon / cartoon representation

The ribbon must also be precomputed offline.

### Required arrays

```text
ribbon_position         float32[n_ribbon_vertices, 3]
ribbon_normal           float32[n_ribbon_vertices, 3]
ribbon_color_chain      float32[n_ribbon_vertices, 4]
ribbon_color_ss         float32[n_ribbon_vertices, 4]
ribbon_index            uint32[n_ribbon_indices]
```

Optional additional arrays:

```text
residue_secondary_structure   uint8[n_residues]
```

Suggested convention:

```text
0 = coil
1 = alpha helix
2 = beta strand
3 = turn
```

### Runtime rendering idea

Use one indexed triangle mesh visual.

First implementation may use a simplified smooth tube or ribbon-like mesh along the C-alpha trace. A more advanced implementation may later encode canonical cartoon shapes for helices and strands.

---

## Optional backbone trace representation

### Required arrays

```text
backbone_position       float32[n_backbone_points, 3]
backbone_color_chain    float32[n_backbone_points, 4]
```

This mode is optional but easy to add and useful for debugging preprocessing.

---

## Coloring modes

The GUI should allow switching among several color sources.

Recommended coloring modes:

- element
- chain
- residue
- secondary structure
- B-factor

To keep runtime simple, these should be **precomputed color arrays** when possible.

For ribbon coloring by secondary structure:

- the ribbon mesh should include a precomputed SS color array.

For surface coloring:

- assign each vertex to its nearest atom or residue during preprocessing,
- then bake chain/element/residue colors into the surface arrays.

---

## Offline preprocessing pipeline

Create a preprocessing tool, for example:

```text
tools/preprocess_protein.py
```

Example usage:

```bash
python tools/preprocess_protein.py 1UBQ --out data/proteins/1ubq
```

This tool may depend on scientific Python packages. The runtime example must not.

---

## Preprocessing stages

### 1. Download raw structure data

Input source: Protein Data Bank, preferably in mmCIF format.

Output example:

```text
raw/1ubq.cif
```

### 2. Parse atomic records

Extract at least:

- atom name
- element
- residue name
- residue index
- chain id
- coordinates
- occupancy
- B-factor
- hetero flag

Filter policy:

- keep protein atoms by default,
- optionally keep ligands,
- discard waters by default,
- resolve alternate locations by best occupancy.

### 3. Normalize coordinates

Compute:

- center
- bounding box
- bounding radius

Store normalized or original coordinates consistently.

Recommended policy:

- store coordinates in original Ångström scale if you want physical radii,
- or store normalized coordinates if the runtime expects normalized geometry.

Either choice is fine as long as the metadata makes camera setup straightforward.

### 4. Assign radii

Need both:

- Van der Waals radii for space-filling,
- reduced radii for ball-and-stick.

### 5. Infer or import bonds

Generate bond connectivity via distance-based rules or a chemistry library.

Output bond endpoints and/or instance parameters.

### 6. Precompute colors

Generate all required color arrays.

### 7. Generate molecular surface

The surface should be computed offline.

Possible approaches:

- use an external tool such as MSMS/EDTSurf/NanoShaper,
- or create a density/distance volume and run marching cubes.

For this example, a robust Python-compatible approximate molecular surface is acceptable.

The result must be a triangle mesh with normals.

### 8. Generate ribbon/cartoon mesh

Build a smooth backbone-derived mesh, preferably using:

- residue ordering,
- chain segmentation,
- optional secondary structure annotations.

The result must be a triangle mesh with normals.

### 9. Optional LOD generation

Optionally save reduced surface or reduced atom sets for future scalability.

### 10. Save bundle

Write `.npz` and `metadata.json`.

---

## Recommended bundle contents

Suggested `.npz` keys:

```text
atom_position
atom_radius_vdw
atom_radius_ball
atom_color_element
atom_color_chain
atom_color_residue
atom_color_bfactor
atom_element
atom_chain
atom_residue

bond_index
bond_position_a
bond_position_b
bond_center
bond_orientation
bond_length_radius
bond_color_element
bond_color_chain

surface_position
surface_normal
surface_color_element
surface_color_chain
surface_color_residue
surface_index

ribbon_position
ribbon_normal
ribbon_color_chain
ribbon_color_ss
ribbon_index

backbone_position
backbone_color_chain

center
radius
bbox_min
bbox_max
```

Suggested `metadata.json` fields:

```json
{
  "pdb_id": "1UBQ",
  "name": "Ubiquitin",
  "source": "RCSB PDB",
  "atom_count": 660,
  "bond_count": 680,
  "chains": ["A"],
  "has_surface": true,
  "has_ribbon": true
}
```

---

## Runtime architecture

The runtime example should be organized roughly as follows:

1. ensure molecule bundle is cached locally,
2. download if missing,
3. load `.npz` + metadata,
4. create a single 3D panel,
5. create an arcball camera/controller,
6. create all visuals for the current molecule,
7. create the SSAO framegraph path,
8. expose ImGui controls,
9. toggle visual visibility and update parameters based on GUI state.

It is acceptable either to:

- create all mode-specific visuals up front and toggle visibility, or
- create/recreate some visuals when the molecule changes.

However, switching **render mode** for a given molecule should ideally not require rebuilding geometry.

---

## Visual groups

For each molecule, the runtime may create the following logical visual groups:

- `atoms_visual`
- `bonds_visual`
- `surface_visual`
- `ribbon_visual`
- `backbone_visual`
- `axes_visual`

The implementation may wrap each group in a small helper object if convenient.

### Suggested visibility table

| Mode | Atoms | Bonds | Surface | Ribbon | Backbone |
|---|---:|---:|---:|---:|---:|
| Atoms | yes | no | no | no | no |
| Ball-stick | yes | yes | no | no | no |
| Surface | optional faint | no | yes | no | no |
| Ribbon | no | no | no | yes | optional |
| Backbone | no | no | no | no | yes |

---

## Camera and interaction

The panel must use a 3D camera with perspective projection.

Recommended initial camera intent:

- target at the molecular center,
- camera far enough to fit the whole structure,
- near/far chosen to avoid clipping.

Required interactions:

- left drag: arcball/orbit rotation,
- right drag or modifier + drag: pan,
- wheel: zoom,
- reset button: restore default view.

Optional:

- autorotation toggle,
- animation update each frame.

---

## Lighting and shading

The geometry pass should provide visually pleasing simple lighting.

Minimum shading model:

- ambient
- diffuse
- mild specular

The exact shader model is flexible. The goal is visual clarity, not physically based shading.

Recommended material controls:

- opacity
- ambient strength
- diffuse strength
- specular strength
- shininess

These may be hardcoded initially if needed.

---

## SSAO requirements

This section is central.

### Architectural requirement

SSAO is not an optional afterthought for this example. It is part of the intended **scene API pressure test**.

The scene layer should therefore be able to represent the SSAO path as a natural set of passes/resources.

### Minimum SSAO pipeline

At minimum, the example should support a pipeline equivalent to:

```text
GEOMETRY PASS
  outputs:
    color texture
    normal texture
    depth texture

SSAO PASS
  inputs:
    normal texture
    depth texture
    noise texture
    kernel samples
  output:
    ssao texture

OPTIONAL BLUR PASS
  input:
    ssao texture
  output:
    blurred ssao texture

COMPOSITE PASS
  inputs:
    color texture
    ssao or blurred ssao texture
  output:
    final panel image
```

### Notes

- The geometry pass should render the active protein visuals into offscreen targets.
- The SSAO pass may be implemented as a fullscreen quad pass or equivalent postprocess visual.
- The blur pass may be separable or single-pass.
- The composite pass may either multiply or mix the base color with ambient occlusion.

### Recommended default composite formula

Conceptually:

```text
final_rgb = base_rgb * (1 - intensity * ao_term)
```

or an equivalent visually stable formula.

The exact formula is not important as long as the result looks good and is controllable.

---

## SSAO resources

### Runtime-generated data

Generate at runtime:

```text
ssao_kernel          float32[n_samples, 3]
ssao_noise_texture   float32[noise_w, noise_h, 3]
```

Recommended default:

- kernel size: 16
- noise size: 4x4

### Offscreen targets

The exact GPU formats can vary, but the architecture should support at least:

```text
color_target
normal_target
depth_target
ssao_target
ssao_blur_target   (optional)
```

The scene API should be able to create or request these per-panel resources.

---

## SSAO uniforms / parameters

The exact implementation may vary, but the SSAO pass will typically need:

- projection matrix
- inverse projection matrix or reconstruct helpers
- viewport size
- sample kernel
- radius
- bias
- intensity
- sample count

Recommended GUI defaults:

- enabled: true
- radius: moderate and scene-scale aware
- bias: small positive value
- intensity: around 1
- samples: 16
- blur: enabled

---

## SSAO applicability by rendering mode

| Mode | SSAO importance | Notes |
|---|---:|---|
| Atoms | medium | can help depth, but may be noisy |
| Ball-stick | medium/high | useful in dense regions |
| Surface | very high | strongest visual gain |
| Ribbon | high | improves fold depth |
| Backbone | low | optional |

Recommended policy:

- enable SSAO by default for `surface` and `ribbon`,
- keep it available for all modes.

---

## Scene/framegraph requirements derived from SSAO

The example should help validate that the scene API can express the following concepts clearly.

### Panel-level framegraph

A panel should be able to declare something conceptually like:

- render pass: protein geometry
- postprocess pass: SSAO
- postprocess pass: blur
- postprocess pass: composite

### Virtual resources

The scene should be able to describe resources such as:

- `main_color_gbuffer`
- `normal_gbuffer`
- `depth_gbuffer`
- `ssao_tex`
- `ssao_blur_tex`
- `final_color`

### Dependencies

The scene/framegraph should be able to express:

- SSAO depends on geometry outputs,
- blur depends on SSAO,
- composite depends on color + AO.

### Postprocessing representation

It should be possible to express postprocessing either as:

- explicit full-screen passes,
- compute passes,
- or a scene-level postprocess abstraction.

This example does not dictate which internal design Datoviz must use. It only requires that the Python-facing API be able to trigger such a pipeline naturally.

---

## What should be easy for an implementation agent

The eventual implementation agent should be able to infer from this spec that Datoviz needs:

- CPU resources for all geometry arrays,
- GPU buffers for vertices/indices/instances,
- some uniform resources,
- offscreen textures for the SSAO path,
- one or more custom shaders or shader variants,
- a framegraph or pass system in the scene layer,
- GUI callbacks that mutate visibility and small parameters.

The exact low-level API names can change.

---

## Suggested implementation order

Because SSAO is intentionally early, the recommended implementation order is:

1. **Build the preprocessing pipeline** for atoms, bonds, colors.
2. **Generate one bundle** for a small protein such as `1UBQ`.
3. **Implement the base 3D panel + arcball camera**.
4. **Implement the geometry pass as an offscreen pass**, not direct-to-screen.
5. **Implement surface rendering** and write color/normal/depth.
6. **Implement SSAO pass**.
7. **Implement composite pass**.
8. **Add ImGui SSAO controls**.
9. **Add ball-and-stick rendering**.
10. **Add ribbon rendering**.
11. **Add multiple molecules + cache download support**.
12. **Add optional blur pass and tuning**.
13. **Add optional backbone mode and additional color schemes**.

This ordering is deliberate: it forces the architecture to accommodate SSAO early.

---

## Performance expectations

The example should remain interactive on ordinary GPUs.

Suggested rough bundle targets:

### Small

- atoms: < 1k
- surface triangles: < 50k

### Medium

- atoms: < 10k
- surface triangles: < 300k

### Large optional

- atoms: < 100k
- surface triangles: up to around 1M if practical

The default startup molecule should be small enough to load quickly and render smoothly.

---

## Error handling and fallback behavior

If a molecule does not provide a specific representation:

- disable the corresponding GUI option, or
- show a clear fallback.

Examples:

- no ribbon mesh available -> ribbon mode disabled
- no surface mesh available -> surface mode disabled

If SSAO is unsupported on a given backend or configuration:

- the example should still render geometry,
- but SSAO controls may be disabled or hidden.

However, for Datoviz v0.4 development, the intended target is that SSAO **is** supported.

---

## Acceptance criteria

The example is considered complete when all of the following are true.

### Functional criteria

- it runs from a clean checkout,
- it downloads missing precomputed bundles from `datoviz/data`,
- it caches them locally,
- it displays a protein in a 3D panel,
- arcball interaction works,
- molecule selection works,
- rendering mode selection works,
- at least one molecule supports atoms, ball-and-stick, surface, and ribbon,
- SSAO is integrated into the rendering pipeline,
- SSAO can be toggled and tuned from ImGui.

### Architectural criteria

- the scene API can represent the offscreen geometry pass,
- the scene API can represent the SSAO pass,
- the scene API can represent the final composite,
- the example does not rely on runtime molecular preprocessing,
- switching representation does not regenerate geometry unnecessarily,
- the implementation is clean enough to serve as a reference example for Datoviz v0.4.

### Visual criteria

- the output is visually attractive,
- the molecule is well framed initially,
- SSAO visibly improves depth perception,
- surface and ribbon modes look clearly 3D.

---

## Non-goals for the first version

The first version does **not** need to include all of the following:

- GPU picking,
- perfect chemically exact surfaces,
- advanced cartoon ribbon semantics,
- trajectory animation,
- high-end physically based rendering,
- huge structures with sophisticated LOD,
- publication-grade labels.

These can be added later.

---

## Minimal first deliverable

A good first deliverable is:

- one molecule (`1UBQ`),
- precomputed atoms/bonds/surface/ribbon bundle,
- one 3D panel,
- arcball camera,
- surface + ball-and-stick modes,
- offscreen geometry pass,
- SSAO pass,
- composite pass,
- ImGui controls for mode, molecule, coloring, and SSAO.

That is already enough to meaningfully validate the v0.4 scene architecture.

---

## Summary

`PROTEIN_ARCBALL_VIEWER` is an example spec whose primary purpose is to validate that Datoviz v0.4 can support a **modern multi-pass scientific visualization scene**, not just single-pass direct rendering.

The key architectural pressure point is **early SSAO integration**.

The example should therefore be designed so that:

- molecular geometry is precomputed offline,
- runtime is lightweight,
- multiple protein representations are available,
- the scene API can describe the full offscreen-to-composite rendering chain,
- the result is attractive, interactive, and easy to understand.
