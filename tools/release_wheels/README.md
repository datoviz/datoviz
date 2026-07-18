# v0.4 Wheel Pipeline

These scripts are the local release-wheel path for v0.4. The primary entry points are the `just`
wheel recipes.

Current local loop:

```sh
just build
just wheel-stage --clean
just wheel-build --platform-tag linux_x86_64 --skip-repair
just wheel-validate --platform-tag linux_x86_64
just wheel-inspect --native-deps
just wheel-check --release-build --precompiled-shaders --cmake-consumer --examples render --render --qt-probe optional
```

Native Linux builds use the neutral `linux_<arch>` tag and skip repair because the host toolchain
may be newer than the release policy. Build release Linux wheels with `just wheel-manylinux-docker`.
The release backend writes a neutral input wheel, then asks `auditwheel` explicitly for
`manylinux_2_34_<arch>`; repair fails if the container or toolchain cannot honestly satisfy that
policy.

Use `--examples basic` for installed Python/C no-window example smokes and `--examples render` for
installed Python/C offscreen render example smokes. The release `rc` machine profile uses
`--examples basic`; the `full` profile uses `--examples render`.

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

Validate a complete wheelhouse against every required platform tag:

```sh
just wheel-validate
```

Validate one locally built artifact:

```sh
just wheel-validate --platform-tag manylinux_2_34_x86_64
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

Run the shared cloud/physical unattended conformance harness against an exact wheel with:

```sh
just wheel-conformance \
  --wheel path/to/datoviz.whl \
  --output-dir build/conformance/machine-id \
  --version 0.4.0rc1 \
  --wheel-run-id 29635132595 \
  --artifact-commit 443adb067 \
  --machine-id machine-id \
  --execution-class physical-interactive \
  --mode physical
```

Physical mode runs the same repeated installed-wheel render profile as cloud mode and leaves the
manual interaction set pending. After the guided checks, record every observation with
`just release-test-approve`. Consolidate any number of returned evidence directories with
`just wheel-conformance-report`; download and open a hosted report with
`just wheel-report <wheel-run-id>`.

The agent-first physical-worker, evidence-submission, coordinator-polling, and combined-report
commands are documented in
[Physical Release Validation](../../docs/contributors/release-physical-validation.md).

The GitHub Actions workflow is stored in `.github/workflows/wheels.yml` and is manual-only through
`workflow_dispatch`. Keep `.github/workflows-draft/wheels.yml` as a staging reference for major
workflow rewrites, but use the live workflow for RC wheel evidence after local validation passes.

Native dependency inspection uses the platform repair tooling expected in CI:

1. Linux: `auditwheel`;
2. macOS: `delocate-listdeps` from `delocate`;
3. Windows: `python -m delvewheel show` from `delvewheel`.
