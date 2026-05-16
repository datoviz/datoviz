# Example Title

> **Agent Pickup**
> - **Category:** `core | api | napari | compute | dashboards | geo | astronomy | bio |
>   engineering | materials | neuro | physics`
> - **Implementation target:** C example, Python example, test fixture, API sketch, or preprocessing
>   script.
> - **Data policy:** inline data, deterministic synthetic data, bundled cache, or public download
>   with cache.
> - **Preprocessing:** none, optional, or required; name the expected script and output files.
> - **Validation:** bounded smoke run, screenshot/readback check, generated fixture, or manual
>   interaction checklist.


## Goal

Describe the user-visible outcome in one paragraph. Make clear whether this is a runnable example,
an API sketch, a fixture target, or a larger polished demo.


## Implementation Target

State the expected artifact:

1. proposed source path, for example `examples/c/example_name.c` or
   `examples/python/example_name.py`,
2. expected command-line arguments,
3. expected outputs such as screenshots, cached data files, or recordings,
4. whether the example must run without network access after the first cache build.


## Data And Asset Plan

Describe every data dependency:

1. preferred public source with URL, citation, license, and expected file names,
2. deterministic synthetic fallback if the download is unavailable or too large,
3. cache directory and cache-file format,
4. size limits for CI, local development, and full-quality showcase mode,
5. integrity checks such as shape, dtype, bounds, checksum, or schema version.


## Preprocessing Pipeline

If preprocessing is required, specify:

1. proposed script path,
2. Python dependencies,
3. input files,
4. output files,
5. coordinate normalization,
6. downsampling, tiling, indexing, or texture packing,
7. reproducibility controls such as random seeds.


## Scene Construction

Describe:

1. panels and cameras,
2. visual families and variants,
3. scene resources and GPU buffers/textures,
4. transforms and coordinate conventions,
5. scales, colormaps, colorbars, labels, annotations, or overlays.


## Runtime Behavior

Describe:

1. animation or streaming loop,
2. interactions and UI controls,
3. picking/probing/selection/readout behavior,
4. update frequency and expected resource churn,
5. fallback behavior when an optional capability is missing.


## Required Datoviz Capabilities

List the active or planned Datoviz features this example needs. Separate mandatory capabilities
from optional polish.


## Implementation Steps

Give an agent a concrete staged plan. The first stage should be small enough to implement and
validate without relying on every stretch feature.


## Validation

Define acceptance criteria:

1. smoke command,
2. expected visual result,
3. screenshot/readback or fixture check,
4. performance target when relevant,
5. manual interaction checklist if automated validation is not practical.


## Open Questions

List unresolved data, API, rendering, licensing, or performance questions. Keep this section empty
only when the file is ready for direct implementation.
