# Datoviz

Datoviz is a GPU-powered visualization engine for scientific data. It helps you build fast,
interactive 2D and 3D views when ordinary plotting tools become too slow or too limited.

Use it when you need to display many points, images, meshes, volumes, annotations, or custom
scientific scenes. Datoviz provides a C scene/app API, a low-level Python interface that accepts
NumPy arrays for supported data uploads, native desktop rendering through Vulkan, and an
experimental browser path through WebGPU/WASM.


## Install

For Python users, the intended v0.4 install command is:

```sh
pip install datoviz
```

During release-candidate testing, the release notes may ask you to use a pre-release command
instead:

```sh
pip install --pre datoviz
```

For C/C++ integration, local development, or platform validation, build from source as described
below.


## Build From Source

Build from source when you need the current development branch, want to contribute, or need local
C/C++ integration before packaged artifacts are available for your platform.

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


## Minimal Python Example

This example creates one point visual with 10,000 random points. Each visual attribute gets one
array: positions, colors, and point diameters.

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


## Which Layer Should I Use?

| Need | Use |
| --- | --- |
| Python code with NumPy arrays and Datoviz visuals | `import datoviz as dvz` |
| Native application or C/C++ integration | C scene/app API |
| Exact low-level C binding access from Python | `datoviz.raw` generated `ctypes` layer |
| Browser rendering for supported examples | experimental WebGPU/WASM subset |
| High-level scientific plotting | VisPy2/GSP when that layer is available |

Datoviz v0.4 is not a compatibility layer for the old Datoviz v0.3 Python plotting API. It is the
lower-level engine that higher-level plotting projects can build on.

See [Choose your layer](https://datoviz.org/start/choose-your-layer/) for more detail.


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
