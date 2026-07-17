# macOS RC1 Physical Validation Handoff

Status: active physical-machine validation. Updated: 2026-07-17.

This handoff is the dispatch for a Codex session running on a physical Mac. Read
[`../../AGENTS.md`](../../AGENTS.md) first, then execute the lane selected by `uname -m`. The user
should be able to start the session with `go`; ask for help only for the short manual interaction
checks or a decision that changes source code or external state.


## Safety And Scope

1. Work on `v0.4-dev`; inspect `git status --short --branch` before pulling.
2. Preserve unrelated user changes. Use `git pull --ff-only` only from a clean compatible checkout.
3. Do not stage or commit `data`, submodule state, build output, wheels, dylibs, captures, or release
   evidence.
4. This lane validates the candidate. Diagnose failures, but propose a focused plan and wait for
   approval before implementing non-trivial fixes.
5. Do not push, dispatch workflows, upload evidence, or publish anything without explicit approval
   in the Mac session.
6. Keep all generated evidence under `build/release/0.4.0rc1/`.
7. Do not install Emscripten or require the strict MkDocs/WebGPU site build in this Mac lane. The
   designated Linux documentation host supplies that proof at the exact release commit. Run only
   lightweight documentation checks here when they are relevant to the source changes.


## Current Release Position

The active candidate version is `0.4.0rc1`. Wheel workflow
[29618922450](https://github.com/datoviz/datoviz/actions/runs/29618922450) passed Linux x86_64 and
aarch64, macOS Intel and arm64, Windows AMD64 and ARM64, and the Python 3.10--3.14 installed-wheel
smokes at `d16512c1d`. Its artifacts are provisional only because the release candidate is now ahead
of that SHA.

Final installed-wheel evidence must use a later all-green workflow whose `headSha` equals the commit
checked out on the Mac. Until that exists, complete the source and physical-runtime preflight and
report the final wheel phase as pending.


## Select The Lane

Record the host facts first:

```sh
uname -m
sw_vers
sysctl -n machdep.cpu.brand_string
git rev-parse HEAD
git status --short --branch
```

- `arm64`: use a descriptive machine ID such as `macos-arm64-m3`; run the Apple Silicon lane and the
  full/manual checks.
- `x86_64`: use machine ID `macos-x86_64-intel`; run the Intel lane and the focused manual checks.
- Any other result: stop and report the unsupported architecture.


## Refresh The Checkout

From a clean checkout:

```sh
git switch v0.4-dev
git pull --ff-only origin v0.4-dev
git submodule update --init --recursive
git status --short --branch
```

If a submodule or the worktree is already dirty, do not overwrite it. Report the exact status and
continue only with checks that preserve the existing work.


## Phase A: Source And Hardware Preflight

Use the repository environment for Vulkan/GLFW/Metal paths when `direnv` is available:

```sh
direnv exec . just build
direnv exec . just test
just spec-check
git diff --check
```

If `direnv` is unavailable, inspect `.envrc` and the build documentation, install or activate the
expected dependencies, and run `just build` / `just test` without silently changing the feature
set. Record the test totals, skips, macOS version, architecture, compiler, GPU, and Vulkan/MoltenVK
facts.

Run bounded native render smokes on both architectures:

```sh
direnv exec . ./build/examples/c/start/scatter --png 10
direnv exec . ./build/examples/c/features/controller_arcball --png 10
direnv exec . ./build/examples/c/features/text_block --png 10
direnv exec . ./build/examples/c/features/mesh_texture --png 10
direnv exec . ./build/examples/c/features/picking --png 10
```

If an example reports missing prepared data, follow its exact preparation instruction only when it
does not mutate the protected `data` submodule; otherwise record an explicit skip. Review generated
captures visually and keep them as build-local evidence.


## Phase B: Physical Interaction

On Apple Silicon, run the complete list. On Intel, run at least `scatter`, `controller_arcball`, and
`text_block`.

Launch each selected example without bounded flags, guide the user through the listed action, and
then close it normally:

1. `./build/examples/c/start/scatter`: resize and pan/zoom.
2. `./build/examples/c/features/controller_arcball`: rotate and zoom the 3D view.
3. `./build/examples/c/features/text_block`: resize and verify text remains legible.
4. `./build/examples/c/features/image_probe`: move the probe and verify the readout.
5. `./build/examples/c/features/mesh_texture`: rotate the textured mesh.
6. `./build/examples/c/features/picking`: exercise picking and close the window.

Record pass/fail and any visual or lifecycle anomaly. Do not classify a launch-only check as an
interaction pass.


## Phase C: Terminal IPython Close And Reopen

This is required on the M1 and useful on Intel. Follow
[`../../docs/how-to/use-ipython.md`](../../docs/how-to/use-ipython.md) using the checkout build:

```sh
DVZ_PYTHON_RUN_DEBUG=1 PYTHONPATH=. ipython
```

Create the documented retained point scene, then verify all of the following:

1. `datoviz.run(scene, figure)` returns a `RunSession` and leaves the prompt responsive.
2. The retained pan/zoom controller responds to mouse pan and wheel zoom.
3. Prompt-side data mutation plus `session.request_frame()` updates the live window.
4. Closing the native window returns cleanly without a spinner or hung process.
5. Reopening the same retained scene creates a responsive window whose pan/zoom still works.
6. `session.close()` is idempotent and final scene destruction exits cleanly.

Keep the `DVZ_PYTHON_RUN_DEBUG` trace. If it hangs, record the last lifecycle line and use
[`HANDOFF_IPYTHON_RUN_CLOSE_HANG.md`](HANDOFF_IPYTHON_RUN_CLOSE_HANG.md) as the resolved-path
investigation reference; do not restart its old implementation plan without new evidence.


## Phase D: Optional Qt/PyQt Proof

Run this on the M1 when Qt development packages and PyQt6 are available. A missing optional provider
is an explicit skip, not permission to change base-wheel dependencies.

```sh
DVZ_CMAKE_ARGS="-DDVZ_ENABLE_QT_BRIDGE=ON" just build
./build/examples/qt/hosted_qt_smoke 120
./build/examples/qt/qt_hosting --smoke-ms 1000
DATOVIZ_QTBRIDGE_LIBRARY=build/qtbridge/libdatoviz_qtbridge.dylib \
  uv run --isolated --with PyQt6 python -m datoviz.qt
DATOVIZ_QTBRIDGE_LIBRARY=build/qtbridge/libdatoviz_qtbridge.dylib \
  uv run --isolated --with PyQt6 python examples/python/qt/hosted_pyqt.py --smoke-ms 1000
```

Record the bridge Qt version, PyQt Qt version, and any clear unsupported-provider diagnostic.


## Phase E: Exact Final Wheel Evidence

Do not run this phase until an all-green `WHEELS` workflow exists at the exact checked-out commit.
Verify eligibility rather than guessing from the latest run:

```sh
git pull --ff-only origin v0.4-dev
head_sha=$(git rev-parse HEAD)
gh run list --workflow wheels.yml --branch v0.4-dev --limit 10 \
  --json databaseId,status,conclusion,headSha,url
```

Select only a run with `status=completed`, `conclusion=success`, and `headSha=$head_sha`. Inspect its
jobs and confirm both Windows, both macOS, and both Linux wheel jobs passed. If no such run exists,
stop this phase and report `final wheel pending`.

Download the host-native artifact into build output, substituting the verified run ID:

```sh
# arm64
gh run download <run-id> --name wheel-macos-arm64 \
  --dir build/release/0.4.0rc1/incoming/macos-arm64

# x86_64
gh run download <run-id> --name wheel-macos-x86_64 \
  --dir build/release/0.4.0rc1/incoming/macos-x86_64
```

Confirm there is exactly one wheel with the expected `macosx_15_0_arm64` or
`macosx_15_0_x86_64` tag. Then run direct installed-artifact validation:

```sh
# M1: required full graphics profile
just release-machine-validate 0.4.0rc1 --wheel <wheel> \
  --profile full --machine-id macos-arm64-m1

# Intel: required RC profile when this available machine is used
just release-machine-validate 0.4.0rc1 --wheel <wheel> \
  --profile rc --machine-id macos-x86_64-intel
```

The validator must prove the installed imports, CLI, native dependency inventory, CMake consumer,
and installed Python/C smokes. The M1 full profile must additionally prove shaderc and offscreen
render smokes. Review `evidence.json`, `environment.json`, all command logs, `failures.md`, warnings,
and captures under:

```text
build/release/0.4.0rc1/evidence/<machine-id>/
```

Do not accept evidence whose wheel checksum differs from the downloaded final artifact.


## Completion Report

Report:

1. architecture, macOS version, CPU/GPU, compiler, Vulkan/MoltenVK, and Python versions;
2. exact commit and, for Phase E, workflow run ID and wheel checksum;
3. build/test totals and bounded capture results;
4. manual interaction and close/reopen results;
5. Qt/PyQt pass or explicit skip reason;
6. evidence directory and final `evidence.json` status, warnings, and failures;
7. any pending final-wheel phase or blocker.

Leave the source worktree clean. Do not commit validation evidence. If a real release blocker is
found, provide the smallest evidence-backed fix plan and wait for approval.
