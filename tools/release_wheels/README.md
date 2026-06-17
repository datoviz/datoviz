# v0.4 Wheel Pipeline

These scripts are the local release-wheel path for v0.4. The primary entry points are the `just`
wheel recipes.

Current local loop:

```sh
just build
just wheel-stage --clean
just wheel-build
just wheel-inspect --native-deps
just wheel-check --cmake-consumer --render --qt-probe optional
```

CI-parity local loop:

```sh
just wheel-ci-local
```

`just wheel-ci-local <platform-tag> 1` also runs `just build` first. The default uses the existing
`build/` tree, which is useful when validating packaging changes without reconfiguring native
build options.

Target RC matrix:

```sh
just wheel-matrix
```

The wheel itself is Python-ABI independent because Datoviz is loaded through `ctypes`, so the
release artifact should be a platform wheel tagged `py3-none-<platform>`. The required Python test
versions are 3.10, 3.11, 3.12, 3.13, and 3.14. Python 3.15 is a prerelease test lane and should be
enabled only in prerelease/nightly wheel builds until the interpreter reaches an RC or final release
appropriate for PyPI users.

The target platform lanes are:

1. Linux `x86_64` and `aarch64`, manylinux 2.34;
2. macOS `x86_64` and `arm64`;
3. Windows `AMD64` and `ARM64`.

The draft GitHub Actions workflow is stored in `.github/workflows-draft/wheels-v04.yml`. It is not
live. Move or copy it to `.github/workflows/` only after the local scripts and one manual branch
run have proven the staged-tree build, wheel inspection, and installed smokes.

Native dependency inspection uses the platform repair tooling expected in CI:

1. Linux: `auditwheel`;
2. macOS: `delocate-listdeps` from `delocate`;
3. Windows: `python -m delvewheel show` from `delvewheel`.
