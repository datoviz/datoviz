# Install

These instructions target Datoviz v0.4. No public release candidate is assumed by this page.

The [release notes index](../releases/index.md) is currently a draft, pre-publication record. Until
it names a published package or artifact, use the [source-build fallback](build-from-source.md).
Check [project status](../reference/project-status.md) for the current release posture.


## Python package

After an RC is published, its release notes will provide an exact version or artifact URL. Create a
virtual environment now, but do not invent a generic pre-release command.

=== "macOS / Linux"

    ```sh
    python -m venv .venv
    source .venv/bin/activate
    python -m pip install --upgrade pip
    # After publication, run the exact command from the linked release notes.
    ```

=== "Windows PowerShell"

    ```powershell
    py -m venv .venv
    .\.venv\Scripts\Activate.ps1
    py -m pip install --upgrade pip
    # After publication, run the exact command from the linked release notes.
    ```

After the final v0.4 package is published, the normal command will be:

```sh
python -m pip install datoviz
```

Verify the active environment:

```sh
python -c "import datoviz as dvz; print('datoviz import ok')"
```

Then continue with the [Quickstart](quickstart.md). Before choosing optional providers or deployment
targets, review [platform support and known limitations](../reference/platform-support.md).


## C and C++

Installed Datoviz packages expose the native library, public headers, runtime assets, and an
exported CMake package:

```cmake
find_package(datoviz CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE datoviz::datoviz)
```

No public installed package is assumed before the RC notes publish one. Until then, follow
[Build from source](build-from-source.md), then use the
[C/C++ integration guide](../how-to/c-integration.md).


## Source fallback

Build from source when no published package exists, when you need the current development branch,
or when you are contributing to Datoviz. The dedicated guide covers prerequisites, shader tools,
vendored dependencies, macOS, Linux, native Windows, WSL2, verification, and example commands:

[Build Datoviz from source](build-from-source.md)


## Package availability

| Path | Current v0.4 posture |
| --- | --- |
| Python RC/final package | Not assumed published; use only a version or artifact linked by published release notes. |
| Source build | Available now for development, C/C++ integration, and package validation. |
| Native Windows wheels | AMD64/ARM64 lanes are validated, but validation is not publication. |
| vcpkg, conda-forge, source bundle | Pre-publication engineering paths; see the draft release notes and project status. |
