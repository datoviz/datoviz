# Allen Mouse Brain IBL-Coordinate Mesh + Volume Plan

> **Agent Pickup**
> - **Category:** `neuro`
> - **Implementation target:** Polished demo concept; implement in stages so the first slice can run with bounded resources.
> - **Data policy:** Public/downloaded assets require cache metadata and an offline fallback or reduced fixture.
> - **Preprocessing:** Usually required; specify source download, conversion, decimation/packing, and generated cache files.
> - **Validation:** Manual visual checklist plus bounded smoke command; add screenshot/readback validation when feasible.


## Summary

Build the next Allen mouse-brain atlas slice by aligning the existing RGBA volume and slice example
with selected Allen CCF structure meshes in IBL `xyz` scene coordinates. The data combines
`data/volumes/allen_mouse_brain_rgba.npy.gz` with public Allen structure meshes, prepared by a small
Python asset step that converts coordinates, applies display normalization, splits or decimates where
needed, and writes explicit cache metadata. The first implementation slice should avoid a full C
coordinate-conversion stack and instead load a root mesh plus a few selected regions beside the
existing volume/slice view. Validate with the bounded smoke command and the visual alignment
checklist for mesh, volume, and anatomical slice movement.

> **Execution Status**
> - **Status:** `PLANNING NOTE`
> - **Updated on:** `2026-05-16`
> - **Purpose:** define the next implementation path for aligning Allen mouse brain volume slices,
>   full 3D volume rendering, and Allen atlas structure meshes in the IBL coordinate system.


## Goal

Extend `examples/c/allen_mouse_brain_slice_glfw.c` from a colored volume/slice demo into a
mesh-aware atlas viewer:

1. load the Allen RGBA volume;
2. load selected Allen CCF region meshes;
3. display volume, embedded slice, cutaway, and surface/context meshes in one coherent 3D space;
4. use the IBL coordinate system as the authoritative scene convention rather than raw Allen voxel
   axes.

The important decision is to make IBL coordinates the canonical scene coordinates. Raw texture
storage may remain in Allen/NumPy memory order, but all displayed geometry and controls should be
derived from the same IBL anatomical coordinate frame.


## Source Findings

The `int-brain-lab/ibl-datoviz` repository contains directly relevant alignment conventions in
`ibl_datoviz/meshes.py` and `ibl_datoviz/viewer.py`.

Key observations:

1. Mesh data is loaded from an IBL cached `meshes.glb` file. Geometry entries are named like
   `<region_id>.obj`.
2. Mesh vertices are converted from Allen CCF coordinates to IBL anatomical coordinates with:

   ```python
   mesh_pos = AllenAtlas().ccf2xyz(mesh.vertices, ccf_order="apdvml")
   ```

3. The `iblatlas` documentation states that Allen mesh vertices use CCF order `(AP, DV, ML)`, while
   `ccf2xyz(..., ccf_order="apdvml")` returns IBL `xyz` coordinates in meters relative to bregma.
4. `ibl-datoviz` normalizes display coordinates with:

   ```python
   display_pos = (pos - offset) * scale
   ```

5. The default `offset` is the mean IBL-coordinate position of root region `997`:

   ```python
   offset = model.load_mesh(997)[0].mean(axis=0)
   ```

6. The default display `scale` is `200`.
7. Hemisphere splitting is done in CCF coordinates before `ccf2xyz()`, using the ML axis in
   `apdvml` order and a hardcoded midline adjustment:

   ```python
   plane_origin = ba.xyz2ccf([0, 0, 0], ccf_order="apdvml")
   plane_origin[2] = 5695
   ```

Useful source links:

- `ibl-datoviz` mesh code:
  https://github.com/int-brain-lab/ibl-datoviz/blob/main/ibl_datoviz/meshes.py
- `ibl-datoviz` viewer normalization:
  https://github.com/int-brain-lab/ibl-datoviz/blob/main/ibl_datoviz/viewer.py
- `iblatlas` coordinate documentation:
  https://docs.internationalbrainlab.org/_autosummary/iblatlas.atlas.html
- Allen CCF structure OBJ meshes:
  https://download.alleninstitute.org/informatics-archive/current-release/mouse_ccf/annotation/ccf_2017/structure_meshes/
- Allen CCF structure PLY meshes:
  https://download.alleninstitute.org/informatics-archive/current-release/mouse_ccf/annotation/ccf_2017/structure_meshes/ply/


## Coordinate Convention

Use IBL `xyz` as the scene-space coordinate convention:

1. `x`: ML coordinate in meters relative to bregma.
2. `y`: AP coordinate in meters relative to bregma.
3. `z`: DV coordinate in meters relative to bregma.

Display-space normalization:

```text
scene_pos = (ibl_xyz - root_mesh_mean_xyz) * 200
```

This should be applied consistently to:

1. region mesh vertices;
2. volume proxy bounds;
3. volume slice plane positions;
4. future probes, cursor readout, point overlays, and insertion tracks.


## Volume Axis Assumption

The current local Allen volume is:

```text
data/volumes/allen_mouse_brain_rgba.npy.gz
shape: (528, 456, 320, 4)
```

Treat this as C-order RGBA data with volume axes:

```text
array[ap, ml, dv, rgba]
```

The GPU texture upload currently sees:

```text
texture.x = dv
texture.y = ml
texture.z = ap
```

because row-major NumPy storage makes the last spatial axis fastest.

This means GUI controls and slice labels should not use raw texture axis names. They should use
anatomical axes:

1. `ML` maps to texture axis `Y`;
2. `AP` maps to texture axis `Z`;
3. `DV` maps to texture axis `X`.

The implementation should verify this with a mesh/volume overlay smoke test rather than relying on
the assumption forever.


## Asset Preparation Strategy

Do not implement the full IBL coordinate conversion in C first. Use a small Python asset-prep script
for the first robust implementation.

Add a script such as:

```text
tools/prepare_allen_ibl_assets.py
```

Responsibilities:

1. import `iblatlas.atlas.AllenAtlas`;
2. import `trimesh`;
3. load selected Allen/IBL mesh geometries;
4. convert vertices with `ba.ccf2xyz(vertices, ccf_order="apdvml")`;
5. compute `root_mesh_mean_xyz` from region `997`;
6. normalize mesh vertices with `(xyz - root_mean) * scale`;
7. export compact C-friendly mesh files for selected regions;
8. write a metadata JSON with:
   - `coordinate_system = "IBL_ML_AP_DV"`
   - `res_um = 25`
   - `offset_xyz_m`
   - `scale`
   - `volume_shape_ap_ml_dv`
   - `texture_axis_for_ml_ap_dv`
   - `volume_bounds_scene`
   - region ids, acronyms, colors, and mesh filenames.

The first mesh subset should be deliberately small:

1. root / whole brain (`997`) for context;
2. cortex or isocortex;
3. hippocampal formation;
4. thalamus;
5. superior colliculus or another visually distinctive target.

Keep raw downloaded Allen/IBL assets out of git. Cache them under `data/` or another local cache path.
Commit only source code, metadata templates if useful, and documentation.


## C Example Integration

Update `allen_mouse_brain_slice_glfw.c` to support optional prepared assets:

```text
./build/examples/c/allen_mouse_brain_slice_glfw \
    --data-file=data/volumes/allen_mouse_brain_rgba.npy.gz \
    --ibl-assets=data/allen_ibl_assets
```

Expected C-side steps:

1. load asset metadata JSON;
2. load selected prepared mesh files;
3. create retained `dvz_mesh()` visuals with lighting enabled;
4. set mesh alpha to a low contextual default, e.g. `40..120`;
5. add root mesh behind the volume, with selectable region meshes in front/context layers;
6. set volume proxy bounds from `volume_bounds_scene`;
7. expose GUI controls using anatomical labels `ML`, `AP`, `DV`;
8. convert slice axis/position from anatomical space to texture axis/normalized coordinate;
9. keep existing volume `MIP` / `COMPOSITE` and cutaway controls.


## Visual Roadmap

Implement features in this order:

### 1. Surface / Context Mesh

Add root mesh and a few selected region meshes first. This is the highest-value alignment test and
the best visual upgrade.

Requirements:

1. root mesh can be shown as translucent anatomical shell;
2. selected region meshes use Allen/IBL colors;
3. opacity is editable;
4. region visibility can be toggled;
5. mesh and volume are visibly aligned under rotation and slice movement.


### 2. Correct Embedded Slice Compositing

Move the slice-plane compositing into the volume raymarch shader so it is physically embedded inside
the volume instead of drawn as a separate transparent visual on top.

Requirements:

1. raymarch front-to-back through the volume;
2. detect ray/slice-plane intersection;
3. sample slice color at the intersection;
4. composite slice color at the correct ray depth;
5. expose `slice_opacity` / `slice_boost`.


### 3. Two-Half Cutaway Mode

Add a mode that renders:

```text
back volume half -> slice plane -> front volume half
```

This is a visualization/presentation mode, not a replacement for embedded slice compositing.

Requirements:

1. camera-aware front/back side selection;
2. side selection relative to anatomical slice plane;
3. controls for keeping positive/negative side;
4. optional mesh clipping to the same plane.


### 4. Preset Visual Modes

Add named presets so the demo is easy to operate:

1. `Slice only`
2. `Volume only`
3. `Slice + volume`
4. `Cutaway + slice`
5. `Surface + slice`
6. `Surface + volume + slice`
7. `Atlas regions`


### 5. Better Transfer Controls

Add controls that are useful for the Allen RGBA volume:

1. alpha gain;
2. alpha gamma;
3. brightness;
4. saturation;
5. threshold;
6. volume opacity;
7. MIP/composite switch;
8. optional color override or anatomical colormap later.


## Validation Plan

Minimum validation after each implementation slice:

```text
just build
./build/testing/dvztest_scene volume
./build/examples/c/allen_mouse_brain_slice_glfw 2 --downsample=2
git diff --check
```

Mesh-alignment-specific validation:

1. load root mesh plus volume with slice at mid-ML;
2. verify root shell encloses the volume;
3. verify AP, ML, and DV slice controls move in anatomically expected directions;
4. verify left/right hemisphere selection matches IBL convention;
5. record one short manual checklist result in this file or a follow-up execution note.


## Open Questions

1. Whether to use IBL `meshes.glb` as the authoritative mesh source or direct Allen
   `structure_meshes/*.obj` / `*.ply` files.
2. Whether prepared mesh assets should be `.npy`, a small custom binary, or simple ASCII OBJ parsed by
   the C example.
3. Whether the current Allen RGBA volume is exactly the same 25um CCF frame as the IBL/Allen meshes.
   The shape strongly suggests it, but mesh overlay should confirm axis and offset alignment.
4. Whether to include optional `iblatlas`-based asset preparation as a developer-only tool or promote it
   to a first-class example data workflow.
