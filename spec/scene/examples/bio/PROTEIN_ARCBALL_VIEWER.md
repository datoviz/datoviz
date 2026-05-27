# Protein Arcball Viewer

> **Example status:** informative pressure test
> **Target:** Python example with prepared protein bundles
> **Data:** bundled cache or public download from `datoviz/data`
> **Validation:** smoke, visual, SSAO, interaction, and performance checks

## Summary

Build an interactive 3D protein viewer with arcball navigation, ImGui controls, multiple molecular
representations, and SSAO integrated from the first serious slice. Runtime loads precomputed
Datoviz-ready molecule bundles; it must not parse raw PDB/mmCIF files or generate molecular
surfaces/ribbons interactively.

The architectural pressure is the offscreen-to-postprocess path:

```text
protein geometry -> color/normal/depth targets -> SSAO -> optional blur -> composite
```

## User-Visible Result

- One window with one 3D panel, dark background, perspective camera, and arcball/orbit controls.
- Default molecule: `1UBQ` or `1CRN`.
- Default mode: `surface` or `ball-and-stick`.
- Coloring: element, chain, residue, secondary structure, or B-factor.
- SSAO enabled by default, with controls for radius, bias, intensity, sample count, and blur.
- Controls for molecule, representation, coloring, opacity, atom scale, bond radius, ribbon radius,
  autorotation, axes, and camera reset.

## Feature Pressure Points

- 3D panel with retained visuals and arcball controller.
- Multiple visual groups toggled without runtime geometry recomputation.
- Instanced sphere/cylinder or sphere-impostor paths.
- Indexed triangle meshes with normals and per-vertex colors.
- Per-panel offscreen resources and multi-pass dependencies.
- Postprocess controls exposed from Python.
- Opaque/transparent stage selection for surfaces and overlays.

## Required Data And Resources

Cache layout:

```text
~/.cache/datoviz/proteins/<pdb_id>/protein.npz
~/.cache/datoviz/proteins/<pdb_id>/metadata.json
```

Recommended molecule set:

| ID | Purpose |
|---|---|
| `1CRN` | Tiny sanity-check protein |
| `1UBQ` | Small default protein |
| `4HHB` | Multichain chain-coloring case |
| `6M0J` or equivalent | Larger surface/ribbon stress case |
| Optional ligand-rich case | Ball-and-stick hetero atom check |

Common metadata:

```text
center float32[3]
radius float32
bbox_min, bbox_max float32[3]
atom_count, bond_count, chain_count uint32
has_surface, has_ribbon bool
```

Required `.npz` arrays:

```text
atom_position float32[n_atoms, 3]
atom_radius_vdw, atom_radius_ball float32[n_atoms]
atom_color_element, atom_color_chain, atom_color_residue, atom_color_bfactor float32[n_atoms, 4]
atom_element uint8[n_atoms]
atom_chain, atom_residue uint32[n_atoms]

bond_index uint32[n_bonds, 2]
bond_position_a, bond_position_b, bond_center float32[n_bonds, 3]
bond_orientation float32[n_bonds, 4]
bond_length_radius float32[n_bonds, 2]
bond_color_element, bond_color_chain float32[n_bonds, 4]

surface_position, surface_normal float32[n_surface_vertices, 3]
surface_color_element, surface_color_chain, surface_color_residue float32[n_surface_vertices, 4]
surface_index uint32[n_surface_indices]

ribbon_position, ribbon_normal float32[n_ribbon_vertices, 3]
ribbon_color_chain, ribbon_color_ss float32[n_ribbon_vertices, 4]
ribbon_index uint32[n_ribbon_indices]

backbone_position float32[n_backbone_points, 3]
backbone_color_chain float32[n_backbone_points, 4]
```

Offline preprocessing may use scientific Python or external molecular tools. It should download raw
structure data, resolve alternate locations, assign radii and bonds, bake color arrays, generate
surface/ribbon meshes, compute metadata, and save the runtime bundle.

## Scene Shape And Runtime Behavior

Visual groups:

| Mode | Atoms | Bonds | Surface | Ribbon | Backbone |
|---|---:|---:|---:|---:|---:|
| Atoms | yes | no | no | no | no |
| Ball-stick | yes | yes | no | no | no |
| Surface | optional faint | no | yes | no | no |
| Ribbon | no | no | no | yes | optional |
| Backbone | no | no | no | no | yes |

Runtime sequence:

1. Resolve or download the selected molecule bundle.
2. Load `.npz` arrays and metadata.
3. Create one 3D panel, arcball camera, and retained visual groups.
4. Fit the camera from `center`, `radius`, and `bbox_*`.
5. Create panel-local `color`, `normal`, `depth`, `ssao`, and optional `ssao_blur` resources.
6. Render active protein visuals into offscreen targets.
7. Run SSAO, optional blur, and final composite.
8. Update only uniforms, visibility, and small parameter resources during interaction.

SSAO defaults:

```text
enabled: true
samples: 16
noise: 4 x 4
radius: scene-scale aware
bias: small positive value
intensity: around 1
blur: enabled
```

If a representation is missing from a bundle, disable that mode or choose a clear fallback. If SSAO
is unavailable on a backend, geometry should still render and the controls should be disabled.

## Minimal Implementation Target

- One molecule, preferably `1UBQ`.
- Precomputed atoms, bonds, surface, and ribbon bundle.
- One 3D panel with arcball camera.
- Surface and ball-and-stick modes.
- Offscreen geometry pass writing color, normal, and depth.
- SSAO pass and composite pass.
- ImGui controls for molecule, mode, coloring, and SSAO.
- No runtime molecular preprocessing.

## Validation / Acceptance Criteria

- Runs from a clean checkout and populates/uses the protein cache.
- Displays a well-framed protein in a 3D panel.
- Arcball rotate, pan, zoom, reset, and optional autorotation work.
- Molecule, representation, coloring, and SSAO controls update the scene without regenerating
  geometry unnecessarily.
- At least one molecule supports atoms, ball-and-stick, surface, and ribbon.
- SSAO visibly improves depth perception in surface/ribbon modes.
- Default small bundle loads quickly and remains interactive; medium bundles remain usable.
- Missing optional bundle arrays fail gracefully.

## Links

- [Shared example policies](../POLICIES.md)
- [Frame plan](../../pipeline/FRAME_PLAN.md)
- [Resource model](../../pipeline/RESOURCE_MODEL.md)
- [Invalidation and caching](../../pipeline/INVALIDATION_AND_CACHING.md)
- [DRP2 specs](../../../drp2/)
