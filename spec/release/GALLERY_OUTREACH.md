# Gallery Outreach

The gallery is release proof and an adoption channel. Scientific showcases should prefer real public
datasets when the data makes a better example than synthetic content.

The goal is a useful domain visualization that also provides documentation, gallery media,
validation pressure, benchmark pressure, social assets, and a concrete feedback request for the
dataset authors.


## Evidence Model

Every gallery example must have a deterministic static screenshot. Screenshots are the canonical
gallery artifact because they are fast to load, easy to review, and suitable for release evidence.

Examples that depend on time, interaction, camera motion, streaming, animation, or
compute-to-render behavior should also provide a short video clip. Videos complement screenshots;
they do not replace them.

Live browser or WebGPU demos may be provided for the supported experimental subset, but they are not
required for every gallery item and must not be the only artifact proving an example. Each live demo
still needs a static screenshot and, when relevant, a recorded clip.

Gallery artifact classes:

1. `screenshot`: required for every example.
2. `video`: recommended for interaction, animation, streaming, compute, and 3D camera examples.
3. `live`: optional, experimental, and limited to supported browser/runtime subsets.

The gallery index should remain useful without JavaScript, GPU access, or video playback. Static
screenshots are therefore the baseline representation for all examples.


## Selection Rules

1. Prioritize recently published open datasets, preprints, challenge data, or public repositories
   with stable links and clear visualization potential.
2. Require license, citation, redistribution, and derived-media terms before committing prepared
   data or publishing generated media.
3. Prefer manageable data sizes and strong structure: images, volumes, point clouds, meshes,
   trajectories, vector fields, time series, graphs, simulations, or geospatial rasters.
4. Make the example useful to the scientist or domain expert, not only decorative.
5. Generate at least one high-quality still image and, when it helps, a short video.
6. Publish a concise gallery page with scientific context, source code, data source, dependencies,
   license, citation, and preprocessing notes.


## Discipline Balance

Balance showcase candidates across:

1. neuroscience, microscopy, and medical imaging;
2. astronomy and cosmology;
3. climate, oceanography, geoscience, and terrain;
4. biology, structural biology, genomics, and biophysics;
5. fluid dynamics, computational physics, and materials science;
6. robotics, motion capture, spatial trajectories, finance, and network science.


## Outreach Policy

1. Maintain a reviewed candidate table before final release.
2. Contact scientists or groups individually with what was visualized, why it may be useful, a
   gallery/media link, and a request for feedback or correction.
3. Ask permission before implying endorsement, quoting responses, or featuring names prominently in
   launch material.
4. Track corrections, attribution requirements, permission status, and follow-up ideas.
5. Reflect any correctness or attribution feedback before public launch when it affects the example.


## Candidate Table

| Field | Purpose |
| --- | --- |
| Scientist / group | Individual or lab to contact. |
| Discipline | Keeps the gallery balanced. |
| Dataset / paper | Data source and scientific context. |
| Release date | Confirms timely outreach. |
| License / terms | Confirms whether data and derived media can be used. |
| Data type / size | Determines feasibility. |
| Datoviz feature fit | Maps data to visuals, interaction, video, GUI, or WebGPU. |
| Planned output | Screenshot, video, notebook, C example, Python example, or web demo. |
| Contact route | Email, lab form, GitHub issue/discussion, or social media. |
| Status | Candidate, approved, implemented, contacted, replied, declined, or published. |


## Owning Specs

Detailed mechanics stay in the existing owners:

1. example coverage and metadata:
   [`../docs/EXAMPLE_COVERAGE.md`](../docs/EXAMPLE_COVERAGE.md);
2. example data/cache rules:
   [`../scene/examples/POLICIES.md`](../scene/examples/POLICIES.md);
3. data submodule, manifests, provenance, promotion, and blockers:
   [`../data/V0_4_DATA_REPOSITORY.md`](../data/V0_4_DATA_REPOSITORY.md).
