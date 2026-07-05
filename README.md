# Datoviz

Datoviz is a C-first GPU rendering engine for scientific visualization. The active `v0.4-dev`
branch is preparing v0.4 release candidates around a retained scene API, DRP2 command streams, and
the native Vulkan/vklite/canvas runtime.

The v0.4 branch is not a compatibility layer for the v0.3 Python plotting API. It is the low-level
engine surface that higher-level projects such as GSP/VisPy2 can target.


## Current Status

The latest published package on PyPI is still v0.3.x. For v0.4 testing, build from source until a
v0.4 release-candidate package is published.

Release-facing v0.4 surfaces:

- C scene/app API in `include/datoviz/`
- installed CMake and pkg-config metadata for C/C++ consumers
- generated `datoviz.raw` Python `ctypes` bindings for the exported C ABI
- top-level `import datoviz as dvz` array-aware direct-engine facade for policy-declared calls
- experimental WebGPU/WASM browser subset for promoted examples
- optional Qt/PyQt hosting through the separately built `datoviz_qtbridge` provider

Explicitly out of scope for v0.4:

- v0.3 source or ABI compatibility
- high-level Python plotting wrappers inside Datoviz
- full WebGPU parity with native Vulkan
- publication-quality PDF/SVG/vector export
- general-purpose compute APIs beyond the narrow experimental compute-to-render path

See [Project status](https://datoviz.org/reference/project-status/) and
[Feature status](https://datoviz.org/reference/feature-status/) for the public status tables.


## Build From Source

Prerequisites:

- Git with submodule support
- CMake 3.21+
- GCC 12+, Clang 15+, or Visual Studio 2022
- Ninja and [`just`](https://github.com/casey/just)
- Python 3.10+
- Vulkan-capable GPU and current graphics drivers
- shader tools from the Vulkan SDK or packages such as `glslang-tools`/`glslangValidator`

```sh
git clone https://github.com/datoviz/datoviz.git --recursive
cd datoviz
git checkout v0.4-dev
just build
just test
```

Editable Python install for local testing:

```sh
pip install -e .
```

`pip install datoviz` currently installs the v0.3 stable package unless you explicitly install a
v0.4 pre-release artifact.


## Minimal Python Example

The top-level Python module keeps C-shaped `dvz_*` names while adapting policy-declared NumPy array
arguments. Use `datoviz.raw` only when you need exact generated `ctypes` signatures.

```python
import numpy as np
import datoviz as dvz

N = 10_000
pos = np.random.uniform(-1, 1, (N, 3)).astype(np.float32)
pos[:, 2] = 0
color = np.random.randint(0, 256, (N, 4), dtype=np.uint8)
color[:, 3] = 255
diameters = np.full(N, 5.0, dtype=np.float32)

scene = dvz.dvz_scene()
figure = dvz.dvz_figure(scene, 800, 600, 0)
panel = dvz.dvz_panel_full(figure)

controller = dvz.dvz_panzoom(scene, None)
dvz.dvz_panel_bind_controller(panel, controller, dvz.DvzDimMaskFlag.DVZ_DIM_MASK_XY)

visual = dvz.dvz_point(scene, 0)
dvz.dvz_visual_set_data(visual, "position", pos)
dvz.dvz_visual_set_data(visual, "color", color)
dvz.dvz_visual_set_data(visual, "diameter_px", diameters)
dvz.dvz_panel_add_visual(panel, visual, None)

dvz.run(scene, figure, title="Scatter plot")
```


## Runtime Model

The active v0.4 path is:

```text
scene frame plans -> DvzSceneFrameArtifact -> DRP2 packet/stream snapshots ->
vklite/canvas/stream -> app
```

Scene emission produces frame artifacts and command stream snapshots consumed by the native runtime
or by the experimental WebGPU/WASM path. DRP2 and low-level runtime APIs are useful for contributors
and backend work, but ordinary users should start from the scene/app APIs.


## Documentation

- [Install](https://datoviz.org/start/install/)
- [Quickstart](https://datoviz.org/start/quickstart/)
- [Examples](https://datoviz.org/examples/)
- [C API reference](https://datoviz.org/reference/c-api/)
- [Python raw ctypes](https://datoviz.org/reference/ctypes/)
- [WebGPU subset](https://datoviz.org/reference/webgpu-subset/)
- [Citation](https://datoviz.org/reference/citation/)
- [Contributing](CONTRIBUTING.md)
- [Build notes](BUILD.md)


## License And Credits

Datoviz is released under the [MIT license](LICENSE). It is developed by
[Cyrille Rossant](https://cyrille.rossant.net/) at the
[International Brain Laboratory](http://internationalbrainlab.org/), with support from the
Wellcome Trust, Simons Foundation, and Chan Zuckerberg Initiative.

If you use Datoviz in research, see [Citation](https://datoviz.org/reference/citation/) for the
current software citation guidance. The final v0.4.0 release will be archived with Zenodo for a
version-specific DOI.

Datoviz builds on earlier open-source GPU visualization work including VisPy, Glumpy, Galry, and
the Vulkan-based Datoviz releases. See the documentation and project papers for background and
citations.
