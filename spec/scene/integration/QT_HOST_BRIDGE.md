# Qt Host Bridge Architecture And Packaging

Status: v0.4 implemented architecture and RC3 packaging contract. This document defines how `datoviz.qt` works with PyQt6 without adding Qt to `libdatoviz`.

Related policy:

1. [OPTIONAL_PROVIDERS.md](OPTIONAL_PROVIDERS.md)
2. [QT_HOSTING.md](QT_HOSTING.md)
3. [HOSTED_BACKENDS.md](HOSTED_BACKENDS.md)


## Problem

The native Qt hosted path already works conceptually:

1. Datoviz creates the Vulkan instance after the adapter supplies Qt's required WSI extensions.
2. Qt adopts that existing instance through `QVulkanInstance::setVkInstance()`.
3. Qt creates or exposes a `VkSurfaceKHR` for the hosted `QWindow`.
4. Datoviz wraps that external surface with the hosted-view API and still owns the runtime.

Qt's C++ API exposes the required `QVulkanInstance` methods:

1. `QVulkanInstance::setVkInstance(VkInstance existingVkInstance)`
2. `QVulkanInstance::vkInstance()`
3. `QVulkanInstance::surfaceForWindow(QWindow*)`

Reference: https://doc.qt.io/qt-6/qvulkaninstance.html

Current Python binding status observed during the May 2026 v0.4 investigation:

| Binding | Observed status |
| --- | --- |
| PyQt6 6.8.x through 6.11.x | `QVulkanInstance` exists; `QWindow.setVulkanInstance()` exists; `QVulkanInstance.setVkInstance()` and `QVulkanInstance.vkInstance()` are missing. |
| PySide6 6.8.x through 6.11.x | The required `QVulkanInstance` and `QWindow` Vulkan binding surface was not exposed in the tested wheels. |

Therefore PyQt6 is viable if Datoviz supplies a small native bridge that calls the missing C++
methods. PySide6 is not a v0.4 target unless future bindings expose enough Qt Vulkan surface to
construct and attach a `QVulkanInstance`.


## v0.4 Decision

Use a separate optional Qt bridge shared library.

Requirements:

1. Do not link Qt into `libdatoviz`.
2. Do not make `import datoviz` depend on Qt or PyQt.
3. Load the bridge only from `datoviz.qt` when PyQt hosting is requested.
4. Build the bridge only when Qt development headers and libraries are available.
5. Check the bridge ABI and Qt runtime version before use.
6. Emit clear diagnostics when the bridge, PyQt Vulkan binding surface, Qt platform support, or WSI
   extension support is missing.

The bridge compensates only for incomplete PyQt bindings. It is not a new renderer and not a new
hosting API.


## Native Bridge ABI

Use the tiny C ABI exported from the separate shared library:

```text
datoviz_qtbridge
```

Provider ABI:

```c
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DVZ_QTBRIDGE_ABI_VERSION 1u

uint32_t dvz_qtbridge_abi_version(void);
const char* dvz_qtbridge_qt_version(void);

int dvz_qtbridge_qvulkan_set_vk_instance(uintptr_t qvulkan_instance, uintptr_t vk_instance);
uintptr_t dvz_qtbridge_qvulkan_vk_instance(uintptr_t qvulkan_instance);

#ifdef __cplusplus
}
#endif
```

Return codes:

| Code | Meaning |
| --- | --- |
| `0` | Success. |
| `-1` | Null `QVulkanInstance*`. |
| `-2` | Null `VkInstance`. |
| `-3` | Qt bridge ABI mismatch or unsupported runtime state. |

The implementation casts `qvulkan_instance` to `QVulkanInstance*` and `vk_instance` to
`VkInstance`, then calls Qt C++:

```cpp
auto* qt_instance = reinterpret_cast<QVulkanInstance*>(qvulkan_instance);
qt_instance->setVkInstance(reinterpret_cast<VkInstance>(vk_instance));
```

`dvz_qtbridge_qvulkan_vk_instance()` is mostly diagnostic, but it proves the adopted instance can be
read back through the same Qt runtime.


## CMake Target

Source layout:

```text
src/qtbridge/
  qt_bridge.cpp
  qt_bridge.h
```

Build option:

```text
DVZ_ENABLE_QT_BRIDGE=ON|OFF|AUTO
```

Source builds default to `AUTO`:

1. if Qt6 Gui development files are found, build `datoviz_qtbridge`;
2. otherwise skip it with a status message;
3. never fail the core Datoviz build because Qt is absent.

The target:

1. link `Qt6::Gui` or the existing `PkgConfig::DVZ_QT6` fallback;
2. not link against `datoviz` unless a concrete need appears;
3. set RPATH/install-name rules so Python can load the installed bridge next to the Datoviz Python
   package or installed library directory;
4. export only the C ABI symbols above.

The existing optional Qt example CMake logic in `examples/qt/CMakeLists.txt` is a useful model for
finding Qt without requiring it.


## Python Integration

`datoviz/qt.py` keeps the public objects:

1. `DatovizWindow`
2. `DatovizWidget`

The `_create_qt_instance()` implementation does not call the missing direct PyQt method:

```python
self._qt_instance.setVkInstance(int(instance))
```

It uses the bridge call:

```python
from PyQt6 import sip

qt_ptr = sip.unwrapinstance(self._qt_instance)
vk_ptr = int(instance)
_qt_bridge.set_vk_instance(qt_ptr, vk_ptr)
```

Bridge loading:

1. first honor `DATOVIZ_QTBRIDGE_LIBRARY` if set;
2. then search installed package/library locations;
3. then search the build-tree location used by local development;
4. raise a `RuntimeError` that explains the missing optional bridge and points to the build option.

The loader should set `ctypes` signatures explicitly:

```python
lib.dvz_qtbridge_abi_version.restype = ctypes.c_uint32
lib.dvz_qtbridge_qt_version.restype = ctypes.c_char_p
lib.dvz_qtbridge_qvulkan_set_vk_instance.argtypes = [ctypes.c_size_t, ctypes.c_size_t]
lib.dvz_qtbridge_qvulkan_set_vk_instance.restype = ctypes.c_int
lib.dvz_qtbridge_qvulkan_vk_instance.argtypes = [ctypes.c_size_t]
lib.dvz_qtbridge_qvulkan_vk_instance.restype = ctypes.c_size_t
```

Use `ctypes.c_size_t` or an explicit pointer-sized integer type on the Python side; do not force
Vulkan handles through 32-bit integer types.


## Runtime Checks

Before constructing the hosted view:

1. confirm PyQt6 imports `QVulkanInstance`, `QWindow`, and `QWindow.setVulkanInstance`;
2. confirm `PyQt6.sip.unwrapinstance()` returns a nonzero pointer for the `QVulkanInstance`;
3. confirm the bridge ABI version equals `DVZ_QTBRIDGE_ABI_VERSION`;
4. call `setVkInstance()`;
5. optionally call `vkInstance()` and compare with the Datoviz `VkInstance`;
6. call `QVulkanInstance.create()`;
7. keep the existing WSI extension validation from `datoviz.qt`.

If a direct PyQt binding appears in a future release, `datoviz.qt` may use it only after proving both
`setVkInstance` and `vkInstance` exist. The bridge remains the v0.4 reliable path.


## PySide Policy

Do not claim PySide6 support in v0.4 unless a specific PySide6 version exposes all of:

1. `QVulkanInstance`;
2. `QWindow.setVulkanInstance()`;
3. `QWindow.vulkanInstance()` or equivalent;
4. `QVulkanInstance.surfaceForWindow()`;
5. a supported way to unwrap `QVulkanInstance*`, such as `shiboken6.getCppPointer()`.

If those exist later, the same native bridge can be reused by passing the unwrapped
`QVulkanInstance*` pointer from Shiboken instead of SIP.


## Release Packaging History And Policy

Main Datoviz wheels should not bundle Qt or PyQt just for this bridge.

RC1 and RC2 disposition and the RC3 packaging route:

1. The main `datoviz` wheel ships the `datoviz.qt` Python adapter and the `datoviz[qt]` extra, but
   not Qt, PyQt, or `datoviz_qtbridge`.
2. Source builds keep `DVZ_ENABLE_QT_BRIDGE=AUTO`: build `datoviz_qtbridge` when Qt development
   files are present and skip it without failing the core build when they are absent.
3. RC1 and RC2 have no packaged bridge provider. Their Qt/PyQt path is source-build-only, and `datoviz[qt]` alone is insufficient because that extra installs PyQt6 but not the native bridge.
4. Local developers and RC testers use `DATOVIZ_QTBRIDGE_LIBRARY` or an installed provider location to point `datoviz.qt` at a source-built bridge.
5. RC3 must provide the first split binary route through conda-forge because it can keep PyQt, Qt, and the bridge on one managed Qt runtime. Local source artifacts are prerequisite evidence, not completion of the release gate.
6. PyPI provider wheels can be considered only after the bridge ABI, Qt runtime-version policy,
   wheel repair behavior, and failure diagnostics are proven on every target OS.

Do not build a Qt-linked bridge into the main manylinux, macOS, or Windows Datoviz wheels unless a
future release policy explicitly accepts the Qt dependency, wheel repair behavior, and RPATH/DLL
layout consequences.


## Split Provider Release Route

The bridge is a provider artifact, not part of the base wheel. RC3 release work is:

1. Require published Qt packages whose QtGui build exposes Vulkan and the public `QVulkanInstance` headers on every claimed target; a locally injected feedstock artifact does not satisfy the release gate.
2. Require a published PyQt6 build against that Qt runtime whose native and cross-built artifacts expose `QVulkanInstance` and `QWindow.setVulkanInstance()`; do not disable the regression test to accommodate an older non-Vulkan Qt artifact.
3. Keep the base wheel proof on `--qt-probe optional` and verify that it installs, imports, and diagnoses missing Qt/PyQt/bridge state cleanly.
4. Add a Qt-capable validation lane that configures with `-DDVZ_ENABLE_QT_BRIDGE=ON`, builds `datoviz_qtbridge`, and fails if Qt6 Gui development files are absent.
5. In that lane, install the known-good published PyQt6 build and run `python -m datoviz.qt` with `DATOVIZ_QTBRIDGE_LIBRARY` pointing at the built bridge.
6. Run the hosted PyQt smoke with the same bridge:

   ```sh
   DATOVIZ_QTBRIDGE_LIBRARY=build/qtbridge/libdatoviz_qtbridge.so \
     python examples/python/qt/hosted_pyqt.py --smoke-ms 1000
   ```

7. Record the exact Qt runtime version reported by PyQt6 and by `datoviz_qtbridge`; mismatches must remain hard failures with clear diagnostics.
8. Exercise negative probes for missing PyQt6, unsupported PyQt/PySide Vulkan bindings, missing bridge library, bridge ABI mismatch, Qt runtime mismatch, and missing platform WSI support.
9. Publish the split provider only after local proof and feedstock CI are green; the package must depend on the same conda-managed Qt/PyQt runtime it links against, and RC3 must validate the resulting exact artifacts on supported hosted platforms.
10. For PyPI, defer provider wheels until there is a documented per-platform policy for bundling or depending on Qt runtime libraries without changing the base `datoviz` wheel contract.

The provider route must never make `import datoviz` depend on Qt, PyQt, or `datoviz_qtbridge`.


## Implemented Core Surface

1. `src/qtbridge/` builds the optional shared provider under `DVZ_ENABLE_QT_BRIDGE=AUTO|ON|OFF` and fails explicitly when `ON` cannot find Qt6 Gui.
2. The installed provider exports the narrow ABI above and is discoverable by `datoviz.qt`, with `DATOVIZ_QTBRIDGE_LIBRARY` retained as the explicit override.
3. `datoviz/qt.py` loads and validates the bridge, checks ABI and Qt runtime compatibility, unwraps `QVulkanInstance`, adopts and reads back the Datoviz Vulkan instance, validates the PyQt surface, and reports missing capabilities without affecting base imports.
4. `python -m datoviz.qt` provides the narrow diagnostic probe, and `examples/python/qt/hosted_pyqt.py` remains the user-facing rendering smoke.
5. Source and local split-package validation cover Linux x86-64 and macOS ARM64, including a packaged bridge, compatible locally built PyQt Vulkan bindings, and base-only imports without PyQt or the provider.
6. Published provider artifacts and supported hosted-platform exact-artifact validation remain the RC3 packaging work defined above.


## Validation

Documentation-only changes:

```sh
git diff --check
git status --short
```

Bridge implementation changes:

```sh
DVZ_CMAKE_ARGS="-DDVZ_ENABLE_QT_BRIDGE=ON" just build
./build/examples/qt/hosted_qt_smoke 120
./build/examples/qt/qt_hosting --smoke-ms 1000
DATOVIZ_QTBRIDGE_LIBRARY=build/qtbridge/libdatoviz_qtbridge.so python -m datoviz.qt
DATOVIZ_QTBRIDGE_LIBRARY=build/qtbridge/libdatoviz_qtbridge.so \
  python examples/python/qt/hosted_pyqt.py --smoke-ms 1000
git diff --check
```

Additional local probes:

```sh
uv run --isolated --with PyQt6 python - <<'PY'
from PyQt6.QtGui import QVulkanInstance, QWindow
print(hasattr(QVulkanInstance, "setVkInstance"))
print(hasattr(QVulkanInstance, "vkInstance"))
print(hasattr(QWindow, "setVulkanInstance"))
PY
```

For a compatible PyQt Vulkan build, the expected result is `False`, `False`, `True`; the bridge covers the first two missing bindings. Older PyQt packages built against a Qt runtime without the Vulkan feature may fail the import itself and are not valid provider dependencies.
