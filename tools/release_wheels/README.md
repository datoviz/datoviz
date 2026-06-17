# v0.4 Wheel Pipeline

These scripts are the local release-wheel path for v0.4. They are intentionally separate from the
older `justfile` wheel recipes while the RC pipeline is being hardened.

Current local loop:

```sh
just build
python tools/release_wheels/stage_wheel.py --clean
python tools/release_wheels/build_wheel.py
python tools/release_wheels/inspect_wheel.py --native-deps
python tools/release_wheels/check_wheel.py --render --qt-probe optional
```

Target RC matrix:

```sh
python tools/release_wheels/wheel_matrix.py
```

The required Python tags are `cp310`, `cp311`, `cp312`, `cp313`, and `cp314`.
`cp315` is a prerelease lane and should be enabled only in prerelease/nightly wheel builds until
the interpreter reaches an RC or final release appropriate for PyPI users.

The target platform lanes are:

1. Linux `x86_64` and `aarch64`, manylinux 2.34;
2. macOS `x86_64` and `arm64`;
3. Windows `AMD64` and `ARM64`.

The draft GitHub Actions workflow is stored in `.github/workflows-draft/wheels-v04.yml`. It is not
live. Move or copy it to `.github/workflows/` only after the local scripts and one manual branch
run have proven the staged-tree build, wheel inspection, and installed smokes.
