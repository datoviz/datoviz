# v0.4 Required Scientific Examples

> **Example status:** release scientific bundle
> **Target:** real-data native C examples with provenance and bounded validation
> **Data:** prepared public or bundled scientific assets with source, license, and preprocessing notes
> **Validation:** smoke, screenshot/video capture, interaction checklist, and provenance review

These examples prove that Datoviz can render real scientific data. They may also be polished enough
for the public gallery, but the lane promise is real data and attribution rather than synthetic
visual impact.


## `showcases_protein`

Flagship current-stack 3D scientific example. It should communicate shaded molecular 3D,
interaction, and multi-pass rendering without waiting for full molecular tooling.

Current v0.4 implementation target: `examples/c/showcases/protein.c`, a prepared RCSB PDB atom
bundle rendered as sphere impostors with arcball camera, EDL, MSAA, SSAO where available, optional
diagnostics, and bounded screenshot smoke.

Source and provenance requirements:

1. source structures come from `https://files.rcsb.org/download/{PDB_ID}.pdb`;
2. the default local cache target is PDB `6M0J`;
3. the repository fallback bundle is `data/examples/proteins/1ubq/prepared`, generated from PDB
   `1UBQ`;
4. `tools/data/prepare_protein_arcball.py 1UBQ --regenerate` records manifest/provenance for the
   fallback bundle;
5. RCSB PDB data usage policy applies.

Defer full ball-and-stick chemistry, labels, picking, and molecular surfaces if needed.


## `showcases_choropleth`

Current-stack geospatial/scientific example for many-region polygon-set rendering, scalar
colormaps, and publication data provenance.

Current v0.4 implementation target: `examples/c/showcases/choropleth.c`, a contiguous U.S. state
population-density choropleth rendered from prepared flat polygon-set arrays. The example uses
Census cartographic state boundaries and Vintage 2025 resident population estimates, with a
cache-local prepared-data fallback and an optional promoted bundle under
`data/examples/us_state_choropleth/prepared`.

Source and provenance requirements:

1. state boundaries come from
   `https://www2.census.gov/geo/tiger/GENZ2024/shp/cb_2024_us_state_20m.zip`;
2. population estimates come from
   `https://www2.census.gov/programs-surveys/popest/tables/2020-2025/state/totals/NST-EST2025-POP.xlsx`;
3. U.S. Census Bureau public data terms apply; cite the Census Bureau as source;
4. `tools/data/prepare_us_state_choropleth.py` records manifest/provenance for the prepared bundle;
5. capture proof still needs a Vulkan-capable visual smoke run before RC publication.
