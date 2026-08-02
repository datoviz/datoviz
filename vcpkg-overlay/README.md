# Datoviz vcpkg Overlay

This overlay is the pre-catalog vcpkg path for Visual Studio and CMake users.

It follows the vcpkg overlay-port contract: the `ports/datoviz` directory contains a
`vcpkg.json` manifest and `portfile.cmake`. A consumer can point vcpkg at this overlay and use
Datoviz through the installed CMake package:

```sh
vcpkg install datoviz --overlay-ports=/path/to/datoviz/vcpkg-overlay/ports
```

```cmake
find_package(datoviz CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE datoviz::datoviz)
```

## Release Source Bundle

Do not point this port at GitHub's auto-generated source archives. Datoviz still needs several
submodule-backed source payloads for v0.4 packaging, including `external/volk`, `external/cimgui`,
and `external/msdf-atlas-gen`. The port expects a release asset named:

```text
datoviz-<version>-source.tar.gz
```

That archive must include the required submodule contents. After publishing the release asset,
replace the placeholder `SHA512` in `ports/datoviz/portfile.cmake`.

Create the bundle with:

```sh
just release-source-bundle 0.4.0
```

The command prints the archive path and SHA512 digest. Use that digest in the vcpkg port after the
asset is uploaded.

## Validation

For pre-tag local validation, point the port at the current checkout:

```sh
DATOVIZ_VCPKG_SOURCE_PATH=$PWD \
  vcpkg install datoviz --overlay-ports=vcpkg-overlay/ports --triplet x64-windows
```

vcpkg sanitizes the port build environment. Preserve the checkout override explicitly, and use classic mode when invoking vcpkg from a directory that contains a top-level `vcpkg.json` manifest. Windows PowerShell:

```powershell
$env:DATOVIZ_VCPKG_SOURCE_PATH = (Get-Location).Path
$env:VCPKG_KEEP_ENV_VARS = "DATOVIZ_VCPKG_SOURCE_PATH"
vcpkg install datoviz:x64-windows --classic --overlay-ports="$PWD\vcpkg-overlay\ports" --triplet x64-windows --binarysource=clear
```

The local checkout contents are not part of vcpkg's ABI key for this environment override. Keep `--binarysource=clear` for source-validation runs so that an older package is not restored after the checkout changes; normal package consumers may use their configured binary cache.

Then validate the generated source bundle path:

```sh
DATOVIZ_VCPKG_SOURCE_URL=file:///tmp/datoviz-source-bundle-smoke/datoviz-0.4.0-source.tar.gz \
DATOVIZ_VCPKG_SOURCE_SHA512=<printed sha512> \
  vcpkg install datoviz --overlay-ports=vcpkg-overlay/ports --triplet x64-windows
```

Minimum proof before publishing the overlay:

```sh
vcpkg install datoviz --overlay-ports=vcpkg-overlay/ports --triplet x64-windows
cmake -S examples/c/integration/cmake_package -B build/vcpkg-consumer \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build/vcpkg-consumer
```

Then repeat on the intended Windows triplets before submitting the official vcpkg catalog PR.
