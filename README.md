# Datoviz

[![Build and test](https://github.com/datoviz/datoviz/actions/workflows/test.yml/badge.svg?branch=v0.4-dev)](https://github.com/datoviz/datoviz/actions/workflows/test.yml)
[![Documentation](https://img.shields.io/badge/docs-datoviz.org-2563eb)](https://datoviz.org/)
[![Python 3.10–3.14](https://img.shields.io/badge/python-3.10%E2%80%933.14-3776ab)](https://datoviz.org/start/install/)
[![License: MIT](https://img.shields.io/badge/license-MIT-22c55e)](LICENSE)

**Datoviz is a GPU-powered visualization engine for interactive scientific data.** Use it when
dense points, images, meshes, volumes, annotations, linked panels, or custom scientific scenes need
more control and performance than an ordinary plotting library provides.

[![Scientific wind-field visualization rendered with Datoviz](docs/images/gallery/showcases/showcase_wind_field.png)](https://datoviz.org/examples/gallery/showcases/showcases_wind_field/)

Datoviz v0.4 provides a retained scene model, native Vulkan rendering, desktop and offscreen
presentation, reproducible capture, and direct use from C or Python. Python users call the generated
binding with `import datoviz as dvz` and pass NumPy arrays directly to supported data-upload APIs.


## v0.4 Release Status

The `v0.4-dev` branch is preparing the first v0.4 release candidate. The native C and Python/NumPy
paths are release-facing; experimental and deferred surfaces remain explicitly labeled throughout
the documentation.

| Surface | v0.4 status |
| --- | --- |
| Native C scene/app API | supported, with feature-specific gaps |
| Python binding with NumPy arrays | supported |
| Offscreen rendering and capture | supported |
| Qt/PyQt hosted rendering | supported, optional provider |
| Retained visual families | supported/experimental by family |
| WebGPU/WASM browser path | experimental |
| Scene compute-to-render integration | experimental |
| DRP2 command streams and runtime internals | advanced/unstable |
| High-level plotting API | external/GSP |

See the detailed [feature status](https://datoviz.org/reference/feature-status/),
[platform support](https://datoviz.org/reference/platform-support/), and
[v0.3 capability disposition](https://datoviz.org/reference/v03-visible-parity/) before relying on
an experimental or backend-specific feature.


## Install

Once a v0.4 release candidate is published on PyPI, use the exact command from its release notes. A
pre-release install will normally look like:

```sh
python -m pip install --pre datoviz
```

After the final v0.4 release, the normal command will be:

```sh
python -m pip install datoviz
```

Until packages are published for your platform, use the source build below. The full
[installation guide](https://datoviz.org/start/install/) covers macOS, Linux, Windows, Python, and
C/C++ integration.


## Minimal Python Example

This deterministic example creates the scatter plot shown in the documentation: 10,000 random
points with random colors and sizes on a dark background.

```python
import ctypes

import numpy as np
import datoviz as dvz

N = 10_000
rng = np.random.default_rng(12345)
pos = np.zeros((N, 3), dtype=np.float32)
pos[:, :2] = rng.uniform(-1, 1, (N, 2))
color = rng.integers(0, 255, (N, 4), dtype=np.uint8)
color[:, 3] = 200
diameter = rng.uniform(4, 12, N).astype(np.float32)

scene = dvz.dvz_scene()
figure = dvz.dvz_figure(scene, 1280, 720, 0)
panel = dvz.dvz_panel_full(figure)
dvz.dvz_panel_set_background_color(panel, dvz.DvzColor(13, 18, 25, 255))

panzoom = dvz.dvz_panzoom(scene, None)
dvz.dvz_panel_bind_controller(panel, panzoom, dvz.DvzDimMaskFlag.DVZ_DIM_MASK_XY)

points = dvz.dvz_point(scene, 0)
dvz.dvz_visual_set_data(points, "position", pos)
dvz.dvz_visual_set_data(points, "color", color)
dvz.dvz_visual_set_data(points, "diameter_px", diameter)

style = dvz.dvz_point_style_desc()
style.aspect = dvz.DVZ_SHAPE_ASPECT_FILLED
style.stroke_width_px = 0
dvz.dvz_point_set_style(points, ctypes.byref(style))
dvz.dvz_visual_set_depth_test(points, False)
dvz.dvz_visual_set_alpha_mode(points, dvz.DVZ_ALPHA_BLENDED)
dvz.dvz_panel_add_visual(panel, points, None)

dvz.run(scene, figure, title="Datoviz")
```

Continue with the annotated [Quickstart](https://datoviz.org/start/quickstart/) or browse the
[example gallery](https://datoviz.org/examples/).


## C And C++

Datoviz is a native C library. Installed packages expose headers and a CMake package for C and C++
applications:

```cmake
find_package(datoviz CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE datoviz::datoviz)
```

The public API uses C linkage and can be called directly from C++. See
[C/C++ integration](https://datoviz.org/how-to/c-integration/) and the generated
[C API reference](https://datoviz.org/reference/c-api/).


## Build From Source

Source builds require Git, CMake 3.21+, Ninja, `just`, Python 3.10+, a supported C/C++ compiler,
shader tools, and a Vulkan-capable runtime. Clone the active branch with its submodules, then build
and test:

```sh
git clone --branch v0.4-dev --recursive https://github.com/datoviz/datoviz.git
cd datoviz
just build
just test
python -m pip install -e .
```

Platform packages, compiler versions, Vulkan/MoltenVK requirements, and native Windows guidance
are maintained in the [installation guide](https://datoviz.org/start/install/). Contributors should
also read [CONTRIBUTING.md](CONTRIBUTING.md) and [BUILD.md](BUILD.md).


## Which Layer Should I Use?

| Need | Use |
| --- | --- |
| Python with Datoviz visuals and NumPy arrays | `import datoviz as dvz` |
| Native application or C/C++ integration | C scene/app API |
| Exact pointer/count form of the Python binding | `datoviz.raw` |
| Browser rendering for promoted examples | experimental WebGPU/WASM subset |
| High-level functions such as `scatter()` or `imshow()` | GSP/VisPy2 when available |

Datoviz v0.4 is the explicit engine layer: scenes, visuals, data uploads, controllers, windows,
captures, and integration surfaces. The old high-level Datoviz Python plotting API is not part of
v0.4; that role belongs to the developing GSP/VisPy2 layer. See
[Choose Your Layer](https://datoviz.org/start/choose-your-layer/) for the complete comparison.


## Documentation

- [Get Started](https://datoviz.org/start/)
- [Examples and Gallery](https://datoviz.org/examples/)
- [How-To Guides](https://datoviz.org/how-to/create-a-scene/)
- [Feature Status](https://datoviz.org/reference/feature-status/)
- [Platform Support](https://datoviz.org/reference/platform-support/)
- [Python With NumPy Arrays](https://datoviz.org/reference/python-direct-engine/)
- [Exact `datoviz.raw` Calls](https://datoviz.org/reference/ctypes/)
- [C API Reference](https://datoviz.org/reference/c-api/)
- [WebGPU/WASM Subset](https://datoviz.org/reference/webgpu-subset/)
- [Changelog](CHANGELOG.md)
- [Citation](https://datoviz.org/reference/citation/)


## License And Credits

Datoviz is released under the [MIT license](LICENSE). It is developed by
[Cyrille Rossant](https://cyrille.rossant.net/) at the
[International Brain Laboratory](https://www.internationalbrainlab.com/), with support from the
Wellcome Trust, Simons Foundation, and Chan Zuckerberg Initiative.

If you use Datoviz in research, follow the current [citation guidance](https://datoviz.org/reference/citation/)
and repository metadata in [CITATION.cff](CITATION.cff). The final v0.4.0 release is planned for
archival with Zenodo and a version-specific DOI.

Datoviz builds on earlier open-source GPU visualization work including VisPy, Glumpy, Galry, and
the Vulkan-based Datoviz releases. Development of v0.4 was assisted by
[OpenAI Codex](https://openai.com/codex/) for implementation, review, testing, documentation, and
release preparation. All changes were directed, reviewed, and validated by the project maintainer.
