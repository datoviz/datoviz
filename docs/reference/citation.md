# Citation

Datoviz v0.4 should be cited as software. The current branch is still pre-RC; cite the repository,
commit, or release-candidate tag you used. The final `v0.4.0` release will be archived with Zenodo
so papers can cite either the exact version DOI or the project concept DOI.


## Preferred Citation

Before the final `v0.4.0` Zenodo archive exists, cite the repository, release candidate, or exact
commit used:

```text
Rossant, C. Datoviz: high-performance rendering for scientific data visualization.
Version or commit: <version-or-commit>. https://github.com/datoviz/datoviz
```

After the final release, prefer the Zenodo version DOI for reproducible work:

```text
Rossant, C. Datoviz: high-performance rendering for scientific data visualization.
Version 0.4.0. Zenodo. https://doi.org/<v0.4.0-version-doi>
```

Use the Zenodo concept DOI when the paper refers to Datoviz as an evolving project rather
than to a specific version.


## BibTeX Template

Use this only after the `v0.4.0` archive is published. Until then, cite the repository, RC tag, or
commit used.

```bibtex
@software{rossant_datoviz_0_4_0,
  author = {Rossant, Cyrille},
  title = {Datoviz: high-performance rendering for scientific data visualization},
  version = {0.4.0},
  date = {<release-date>},
  publisher = {Zenodo},
  doi = {<v0.4.0-version-doi>},
  url = {https://github.com/datoviz/datoviz}
}
```


## Methods Text

After the final release, use wording like:

```text
Interactive scientific visualization was implemented with Datoviz v0.4.0, a GPU-powered
visualization engine for large scientific datasets.
```

Before then, replace `v0.4.0` with the exact commit, development version, or release-candidate tag
used.

If your result depends on an experimental backend or low-level interface, name it explicitly. For
example, mention the WebGPU/WASM subset or the generated `datoviz.raw` bindings when those details
matter for reproducibility.


## Journal Paper

A Journal of Open Source Software draft is maintained in `paper/`. If accepted, the
JOSS paper citation should become the preferred scholarly citation, while the Zenodo DOI
remains the preferred version-specific software archive citation.


## Release Metadata

The repository-level citation metadata lives in `CITATION.cff`. The final release pass
must update it with the exact `date-released` and DOI after Zenodo archives `v0.4.0`.
