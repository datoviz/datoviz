# Crystal Phonon Explorer

> **Agent Pickup**
> - **Category:** `materials`
> - **Implementation target:** Scientific domain example with staged implementation, starting from deterministic or prepared data.
> - **Data policy:** Synthetic or prepared-cache first slice; real public data may be a second stage with license notes.
> - **Preprocessing:** Document any data conversion script, output schema, coordinate normalization, and cache validation.
> - **Validation:** Smoke run plus domain-specific visual, picking/probe, and performance acceptance criteria.


## Example Name

`CRYSTAL_PHONON_EXPLORER`


## Purpose

Specify a Datoviz v0.4 showcase example for interactive materials-science visualization of a
periodic crystal animated by phonon-like lattice vibrations.

This example should not be a generic atom viewer. The existing protein viewer already pressures
arcball interaction, element coloring, atom-like primitives, and molecular rendering. This example
exists to cover a different scientific structure:

- periodic lattices,
- replicated supercells,
- unit-cell and lattice-vector overlays,
- sublattice-specific motion,
- coherent time-dependent displacement fields,
- optional defect-localized perturbations,
- linked mode/frequency panels.

The first implementation may use simulated phonon modes. Real phonon eigenvectors can be added later
through an offline preparation step.


## Why This Example Exists

The example is intended to pressure the v0.4 scene architecture in ways that are distinct from
`PROTEIN_ARCBALL_VIEWER`:

1. structured 3D geometry generated from a small periodic basis,
2. buffer updates for thousands to hundreds of thousands of animated atom positions,
3. multiple synchronized visuals derived from the same crystal state,
4. stable 3D picking while positions change every frame,
5. linked UI and plot panels controlling a live 3D scene,
6. optional offline animation/video export of deterministic motion.

The default should look like a materials-science visualization rather than a chemistry or biology
viewer.


## Recommended Default Scenario

Use a perovskite soft-mode crystal, such as idealized `SrTiO3`.

Default scene:

- a `5 x 5 x 5` or `6 x 6 x 6` supercell,
- atoms colored by element,
- translucent unit-cell or supercell boundary,
- optional TiO6 coordination octahedra,
- animated optical mode where Ti and O sublattices move against each other,
- ghost/rest positions shown faintly,
- displacement arrows shown at a sparse subset of sites,
- bottom panel showing a small phonon-mode spectrum or mode selector.

This gives the example a clear materials-specific identity:

```text
periodic crystal -> phonon mode -> animated supercell -> linked mode controls
```


## Dataset Strategy

### Stage 1: Deterministic Simulated Data

The initial example should work without a network request or API key. It may generate an idealized
perovskite structure directly in the preparation script or runtime example.

Minimal logical data:

```text
lattice             float32[3, 3]
basis_frac          float32[N_basis, 3]
species             uint8[N_basis]
species_name        string[N_species]
species_color       uint8[N_species, 4]
species_radius      float32[N_species]
supercell_shape     uint32[3]
mode_displacement   float32[N_atoms, 3]
mode_phase          float32[N_atoms]
mode_frequency      float32
```

Derived runtime arrays:

```text
rest_position       float32[N_atoms, 3]
animated_position   float32[N_atoms, 3]
atom_color          uint8[N_atoms, 4]
atom_radius         float32[N_atoms]
bond_index          uint32[N_bonds, 2]       # optional
cell_edges          float32[N_edge_vertices, 3]
arrow_position      float32[N_arrows, 3]     # optional sparse displacement arrows
arrow_vector        float32[N_arrows, 3]
octa_mesh_position  float32[N_poly_vertices, 3]
octa_mesh_index     uint32[N_poly_indices]
```

The generated data should be deterministic so screenshots and tests remain reproducible.


### Stage 2: Prepared Real Data

A later preparation script may load real crystal and phonon data from tools such as Phonopy,
Materials Project, NOMAD, or another curated source, then export the same Datoviz-ready cache
schema.

Runtime should still load a compact prepared cache rather than requiring live API access.

Suggested cache layout:

```text
~/.cache/datoviz/materials/perovskite_soft_mode/
  metadata.json
  rest_position_f32.bin
  species_u8.bin
  color_rgba8.bin
  radius_f32.bin
  mode_displacement_f32.bin
  mode_phase_f32.bin
  bond_index_u32.bin
  cell_edges_f32.bin
  octa_mesh_position_f32.bin
  octa_mesh_index_u32.bin
```


## Scene Layout

Recommended window layout:

```text
+------------------------------------------------------------------+
| 3D crystal viewport                                               |
| animated atoms, rest ghosts, unit-cell edges, optional octahedra   |
|                                                        controls    |
+------------------------------------------------------------------+
| phonon mode / frequency panel                                     |
+------------------------------------------------------------------+
```

Minimum viable version:

1. one 3D panel with arcball interaction,
2. animated atoms,
3. unit-cell or supercell wireframe,
4. GUI controls for mode, amplitude, frequency, and supercell size,
5. hover or click readout for one atom.

Preferred fuller version:

1. 3D panel with atom positions updated every frame,
2. faint ghost lattice at rest positions,
3. sparse displacement arrows,
4. optional translucent coordination polyhedra,
5. linked lower panel with mode/frequency markers,
6. selected atom and nearest-neighbor shell highlight.


## Visual Families

The example should use the active v0.4 scene families where possible:

- `point` or future sphere impostor visual for atoms,
- `primitive` or mesh visual for octahedra and optional atom spheres,
- `path` or line/strip visual for unit-cell edges and lattice vectors,
- `path` or future vector/glyph visual for displacement arrows,
- `image` or path/line visuals for the linked spectrum panel,
- narrow text/annotation support for selected atom readout when available.

If true sphere impostors are unavailable, atom points may be used for the first implementation, with
radius encoded through point size.


## Phonon Motion Model

The default simulated motion is:

```text
position_i(t) = rest_i + amplitude * displacement_i * sin(omega * t + phase_i)
```

Where:

- `rest_i` is the equilibrium Cartesian position,
- `displacement_i` is the mode displacement vector,
- `phase_i` may encode the wave vector through the supercell,
- `omega` is derived from the selected mode frequency.

Suggested simulated modes:

1. **Longitudinal Acoustic**
   - phase advances along a lattice axis,
   - displacement parallel to the wave vector,
   - all sublattices move coherently.

2. **Transverse Acoustic**
   - phase advances along a lattice axis,
   - displacement perpendicular to the wave vector,
   - visually distinct shear-like motion.

3. **Optical Sublattice**
   - cations and anions move against each other,
   - good default for a perovskite because it is immediately recognizable.

4. **Soft Ferroelectric Mode**
   - Ti atoms shift inside O octahedra,
   - a polarization arrow can show the net displacement direction.

5. **Defect-Localized Mode**
   - optional mode around one oxygen vacancy or dopant,
   - displacement amplitude decays with distance from the defect.


## Controls

Recommended controls:

```text
Material:        SrTiO3 / Si / Graphene / custom cache
Mode:            Longitudinal / Transverse / Optical / Soft / Defect
Play:            checkbox
Amplitude:       slider
Frequency:       slider
Supercell:       1x1x1 / 3x3x3 / 5x5x5 / 7x7x7
Show rest:       checkbox
Show bonds:      checkbox
Show cell:       checkbox
Show arrows:     checkbox
Show octahedra:  checkbox
Atom scale:      slider
Auto rotate:     checkbox
Reset camera:    button
Export clip:     button or command-line option
```

The exact GUI API may vary; the behavior is the important part.


## Picking And Readout

Picking should identify the nearest atom in the current animated frame or the nearest rest-position
site if animated picking is not yet available.

Readout should include:

```text
atom index
element
unit-cell index
fractional coordinate
cartesian coordinate
current displacement magnitude
coordination count
```

Selecting a Ti atom in `SrTiO3` should optionally highlight its oxygen octahedron and nearest O
neighbors. This makes the example clearly periodic-material-specific.


## FramePlan Shape

### Static Setup

Initial frame:

```text
UploadNode  -> rest atom positions
UploadNode  -> animated atom positions, initially equal to rest positions
UploadNode  -> atom colors and sizes
UploadNode  -> unit-cell edge vertices
UploadNode  -> optional bond, arrow, and octahedron geometry
RenderNode  -> panel: rest ghosts, animated atoms, cell edges, optional overlays
```


### Animated Frame

Each animation frame updates the atom position buffer and any dependent arrow buffers:

```text
UploadNode  -> animated atom positions
UploadNode  -> sparse arrow vectors, if arrows are visible
RenderNode  -> panel: crystal visuals
RenderNode  -> panel: linked mode/frequency plot, if time cursor moves
```

The unit-cell edges, static rest ghosts, bonds, and octahedron topology should not be re-uploaded
unless the material, supercell shape, or visibility/data mode changes.


### Supercell Change

Changing the supercell size invalidates the generated structure:

```text
UploadNode  -> regenerated rest positions
UploadNode  -> regenerated animated positions
UploadNode  -> regenerated colors, sizes, bonds, cell edges, optional polyhedra
RenderNode  -> panel: crystal visuals
```

The implementation should avoid retaining stale pointers into regenerated arrays across this update.


## DRP2 Command Categories

The example is expected to require:

- buffer creation for positions, colors, sizes, indices, and optional overlays,
- repeated buffer uploads for animated positions,
- draw commands for points/primitives/paths/meshes,
- panel transform push updates from the arcball camera,
- optional readback/pick requests,
- optional video/capture commands through the app/canvas layer.

It should not require a parallel renderer path.


## Implementation Notes

The first C implementation can be deliberately modest:

1. generate the perovskite supercell in C or through a small preparation script,
2. keep rest and animated position arrays in owned runtime memory,
3. recompute animated positions each frame from deterministic mode parameters,
4. upload only changed arrays during animation,
5. use current point/path/primitive capabilities rather than waiting for ideal sphere impostors.

Python bindings and real phonon file support can come later. The scientific and architectural shape
should remain the same.


## Key Pressure On The Scene Spec

This example checks that Datoviz v0.4 can express a live, structured, scientific 3D scene where:

- one compact periodic dataset expands into many renderable resources,
- animation changes geometry every frame without rebuilding static resources,
- linked panels and controls modify the same scene state,
- picking remains meaningful despite animated positions,
- the scene stays deterministic enough for tests and offline video export.
