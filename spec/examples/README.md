# Examples And Gallery Policy

> **Status:** normative v0.4 policy
> **Scope:** executable examples, generated example pages, gallery selection, and scientific
> showcase expectations

Datoviz examples are executable project assets. Their source belongs under `examples/`, not under
`docs/`. The documentation site may generate pages, screenshots, animations, indexes, and gallery
views from those examples, but it should not be the authoritative home of runnable example code.


## Goals

The examples system serves four separate user needs:

1. teach one API feature with the smallest useful program;
2. teach one visual family with the smallest useful scene;
3. present a polished, curated public gallery with a consistent graphic direction;
4. prove scientific relevance with real datasets, real encodings, and provenance.

These needs overlap, but they should not be collapsed into one bucket. A beautiful showcase does not
replace a minimal copy-safe feature example. A real-data scientific example does not need to match
the curated gallery theme perfectly, but it still needs clear styling, honest encodings, and source
attribution.


## Source Layout

Use these logical source categories for new first-class examples:

```text
examples/
  features/
  visuals/
  showcase/
  scientific/
```

If a build system, language binding, or runtime requires a compatibility prefix such as
`examples/c/` or `examples/webgpu/`, preserve that path while exposing the same logical category in
metadata. The category names above are the public taxonomy used by documentation, release notes,
search, and gallery tooling.


## Categories

| Category | Unit | Purpose | Data |
| --- | --- | --- | --- |
| `features` | one API feature | teach one capability quickly | usually synthetic |
| `visuals` | one visual family | teach one visual type with minimal surrounding setup | usually synthetic |
| `showcase` | one polished composition | provide curated visual impact and design consistency | synthetic or domain-flavored fake data allowed |
| `scientific` | one real dataset story | prove relevance to scientific users | real data with provenance |

Feature examples should stay minimal. They demonstrate a capability such as a colorbar, controller,
pick query, texture upload, linked panel, or partial update.

Visual examples should stay minimal. They demonstrate a public visual family such as point, marker,
path, image, mesh, sphere, volume, text, or labels. They may use a few supporting features, but only
when needed to make the visual family intelligible.

Showcase examples are allowed to compose visuals, panels, styling, annotations, camera motion, and
synthetic or procedural data. Their job is to make the project look coherent and modern. They must
not imply that fake data is scientific evidence.

Scientific examples use real datasets and domain-appropriate encodings. They may be less visually
uniform than curated showcases because real scientific data varies in shape, scale, and convention.
They should still share baseline Datoviz presentation quality: clean framing, legible colorbars,
consistent typography where applicable, clear backgrounds, documented units, and stable captures.


## Website Pages

The generated public documentation should separate exhaustive example discovery from editorial
gallery presentation:

```text
/examples/        complete index of all documented examples
/examples/<id>/   one generated page per source example
/gallery/         curated best-of subset
```

Every documented example should have a generated page. That page should include:

1. title and short description;
2. screenshot or animation;
3. source code or a direct source link;
4. run or capture command;
5. category, tags, status, backend requirements, and copy-safety metadata;
6. related examples when useful;
7. dataset source, license, citation, and encoding notes for scientific examples.

The gallery is not an exhaustive thumbnail wall. It is an editorial selection chosen for visual
quality, variety, relevance, and release messaging. The examples index is where every example
appears.


## Gallery Selection

Default gallery behavior:

| Category | Gallery default |
| --- | --- |
| `features` | excluded |
| `visuals` | excluded unless promoted |
| `showcase` | included when visually mature |
| `scientific` | included when visually mature and properly attributed |

Promoted gallery entries should have strong screenshots or short loops, stable captures, clear
metadata, and no unsupported implication about release status. A front-page gallery may select an
even smaller subset than `/gallery/`.


## Metadata Contract

Every documented example should eventually expose machine-readable metadata. The exact storage
format may be a manifest, structured source comment, or nearby sidecar file, but the fields should
support documentation generation, filtering, release proof, and LLM retrieval.

Required baseline fields:

```yaml
id: visual.point
title: Point visual
category: visuals
summary: Minimal retained point visual.
source: examples/visuals/point.c
tags: [point, scene, panzoom]
status: supported
backends: [native]
copy_safe: true
gallery:
  include: false
screenshots:
  - docs/assets/examples/visuals/point.png
validation:
  - just test scene
```

Scientific examples require additional provenance and encoding fields:

```yaml
dataset:
  name: Allen Mouse Brain atlas
  source: https://example.invalid/dataset
  license: CC-BY-4.0
  citation: Example citation
encoding:
  position: registered 3D coordinates
  color: anatomical region
  units: micrometers
```

Avoid hiding important metadata only in prose. Free-form docstrings are useful for the generated
page introduction, but category, tags, gallery inclusion, backend requirements, copy safety, and
dataset provenance should be structured.


## Data Policy

Synthetic or procedural data is acceptable for `features`, `visuals`, and `showcase` examples when
the purpose is API clarity, visual design, or deterministic testing. The generated page should avoid
scientific claims for such examples.

Scientific examples should prefer public datasets with clear licenses. They must record source,
license, citation, and any preprocessing needed to reproduce the displayed data. Large or derived
assets must follow the repository data-submodule and generated-binary rules in
[`../data/README.md`](../data/README.md) and
[`../data/V0_4_DATA_REPOSITORY.md`](../data/V0_4_DATA_REPOSITORY.md).


## Related Specs

- [`../docs/EXAMPLE_COVERAGE.md`](../docs/EXAMPLE_COVERAGE.md) tracks required feature and visual
  coverage.
- [`../docs/GALLERY_SITE.md`](../docs/GALLERY_SITE.md) records public site implementation notes.
- [`../release/GALLERY_OUTREACH.md`](../release/GALLERY_OUTREACH.md) owns outreach and scientific
  showcase candidate policy.
- [`../scene/examples/PLANNING.md`](../scene/examples/PLANNING.md) tracks scene-specific example
  staging and release priorities.
