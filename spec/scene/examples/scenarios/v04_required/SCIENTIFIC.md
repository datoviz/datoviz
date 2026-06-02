# v0.4 Required Scientific Examples

> **Example status:** release scientific bundle
> **Target:** real-data native C examples with provenance and bounded validation
> **Data:** prepared public or bundled scientific assets with source, license, and preprocessing notes
> **Validation:** smoke, screenshot/video capture, interaction checklist, and provenance review

These examples prove that Datoviz can render real scientific data. They may also be polished enough
for the public gallery, but the lane promise is real data and attribution rather than synthetic
visual impact.


## `protein_arcball_viewer`

Flagship current-stack 3D scientific example. It should communicate shaded molecular 3D,
interaction, and multi-pass rendering without waiting for full molecular tooling.

Current v0.4 implementation target: `examples/c/scientific/protein.c`, a prepared RCSB PDB atom
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
