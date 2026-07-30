# Gallery Reference Screenshot Policy

## Contract

The canonical v0.4 native gallery PNGs are generated on one designated physical Linux reference host. A screenshot produced on macOS, Windows, hosted CI, another Linux machine, another Vulkan physical device, or another driver is comparison evidence only and must not silently replace the Linux baseline.

The reference identity includes the declared host label, Linux distribution and kernel, selected Vulkan physical-device index, device name and UUID, driver name, version and UUID, Vulkan summary hash, source commit, `data` commit, capture-tool hashes, relevant Vulkan loader environment, and exact capture command. The default Datoviz GPU context selects Vulkan physical-device index zero, so provenance records GPU0 as the selected device and preserves the complete `vulkaninfo --summary` output.

## Candidate Workflow

Run `just gallery-reference-candidates` from a clean source tree and clean `data` submodule on the designated Linux host. The workflow refuses non-Linux execution and any output directory outside a proper child of `build/`. It captures the complete reviewed screenshot set twice in serial order by default, validates every PNG, compares the two Linux runs for byte repeatability, compares the first Linux run with the committed canonical PNG, and writes `build/gallery-reference/report.json`, `provenance.json`, enhanced differences, and `index.html`.

The two independent Linux runs must be byte-identical before promotion. A repeat mismatch is an unstable-capture blocker even when the difference fits the looser cross-environment pixel-equivalence tolerance.

## Review and Promotion Boundary

Candidate PNGs, repeat PNGs, enhanced differences, and HTML reports remain ignored build-local artifacts. They are not canonical merely because generation succeeded.

After visual review, the maintainer must explicitly approve the exact candidate set before any PNG is copied into `data/gallery/v0.4/`, any `data` submodule commit is created, or the parent gitlink is updated. Promotion should retain the reviewed report and provenance as release evidence and should commit a machine-readable provenance record with the approved canonical set.

No image, `data` commit, parent gitlink, push, website asset, or deployment may be published as part of candidate generation.
