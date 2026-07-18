# Physical Release Validation

Use this procedure for every release candidate and final release. Durable requirements live in
[`spec/release/PHYSICAL_VALIDATION.md`](../../spec/release/PHYSICAL_VALIDATION.md); current candidate
state and pending machine work live in `agents/now/`.


## Inputs And Safety

Set the concrete release version, candidate commit, wheel-workflow run ID, host wheel, and stable
machine ID before starting. Work from a compatible clean checkout at the candidate commit and
preserve unrelated changes. Keep wheels, captures, logs, and evidence under
`build/release/<version>/`; do not stage them.

Do not push, dispatch workflows, upload evidence, or publish during physical validation without the
explicit approval required by the repository rules.


## 1. Verify The Candidate Artifact

Record host facts, `git rev-parse HEAD`, and `git status --short --branch`. Inspect the selected
successful `wheels.yml` run and require its `headSha` to equal the candidate commit. Record the wheel
filename and SHA-256 before installation.

Install only that wheel into a new virtual environment. Run the applicable installed validator:

```sh
just release-machine-validate <version> --wheel <wheel> --profile rc --machine-id <machine-id>
```

Use `full` on the designated graphics-capable host. A validation pack may instead run
`./validate-rc.sh`, `./validate-full.sh`, or `./validate.ps1 -Profile rc`. Retain the resulting
evidence directory.


## 2. Run The Exact-Wheel Quickstart

Run the Quickstart from a directory outside the checkout using the fresh wheel environment. Copying
the fixture to a disposable directory prevents an accidental checkout import.

Linux and macOS:

```sh
release_scratch=$(mktemp -d)
cp /path/to/datoviz/examples/docs/quickstart.py "$release_scratch/quickstart.py"
cd "$release_scratch"
/path/to/wheel-venv/bin/python quickstart.py
```

Windows PowerShell:

```powershell
$releaseScratch = New-Item -ItemType Directory -Path ([IO.Path]::GetTempPath()) `
  -Name ("datoviz-release-" + [guid]::NewGuid())
Copy-Item C:\path\to\datoviz\examples\docs\quickstart.py $releaseScratch\quickstart.py
Set-Location $releaseScratch
& C:\path\to\wheel-venv\Scripts\python.exe .\quickstart.py
```

Ask the maintainer to resize, pan, zoom, and close the window normally. Record `pass` only after all
four actions succeed. Retain the wheel checksum beside this result.


## 3. Run The Fixed Native Interaction Set

Build the checkout at the same candidate commit. Launch each executable without `--png`,
`--smoke-ms`, or other bounded flags. State the requested action, wait for the maintainer's result,
then close the program before launching the next one.

| Example | Maintainer action |
| --- | --- |
| `start/scatter` | Resize; pan and wheel-zoom. |
| `features/controller_arcball` | Rotate and zoom the 3D view. |
| `features/text_block` | Resize; verify text remains legible and correctly laid out. |
| `features/image_probe` | Move the probe; verify the readout changes coherently. |
| `features/mesh_texture` | Rotate the textured mesh; verify texture and depth remain correct. |
| `features/picking` | Exercise picking, resize, close, reopen once, and close again. |

The checkout command is:

```sh
./build/examples/c/<example>
```

These rich examples are currently checkout-built because the wheel does not package interactive
examples. Report them as source live-smoke evidence paired with the exact-wheel Quickstart and
installed validator. Do not describe them as wheel-contained examples.

On a constrained but supported host, `scatter`, `controller_arcball`, and `text_block` are the
minimum focused set. Record every omitted example as `skip` with its reason.


## Platform Additions

### Linux

Record distribution/version, architecture, GPU, Vulkan implementation, and driver. Use `just build`
for the native examples. A host without a desktop session may run automated offscreen validation,
but its manual profile is `skip`; offscreen output does not satisfy live interaction.

Linux x86_64 with a Vulkan GPU is required. Linux aarch64 execution is required when a native host
is available.

### macOS

Record `uname -m`, `sw_vers`, CPU, GPU, Vulkan/MoltenVK, and compiler facts. Use `direnv exec .` for
graphics commands when available. Apple Silicon runs the full interaction set; Intel runs at least
the focused set and records other checks explicitly.

On Apple Silicon, also perform the documented IPython `RunSession` close/reopen test from
[`use-ipython.md`](../how-to/use-ipython.md). When Qt development packages and PyQt6 are present,
run the optional native Qt and hosted PyQt smoke checks; otherwise record an explicit provider skip.

The designated Linux documentation host supplies the strict MkDocs/WebGPU build. Do not install
Emscripten merely to repeat that proof on a Mac.

### Windows

Record Windows version, architecture, GPU, and driver. Run the PowerShell validator before the
interactive phase. Treat an assertion dialog, hidden crash dialog, or hung process as a failure and
record its text; a zero exit observed only after dismissing a dialog is not a pass.

Windows AMD64 is required. Windows ARM64 execution is required when a native host is available; do
not execute an ARM64 wheel on an x64 host.


## 4. Record And Return Evidence

Record:

1. machine ID, OS/architecture, GPU, driver, and runtime facts;
2. release version, candidate commit, workflow run ID, wheel filename, and checksum;
3. installed-validator profile and evidence path;
4. exact-wheel Quickstart result;
5. one result for every selected native example;
6. resize, close, reopen, visual, and lifecycle observations;
7. explicit skips and blocking failures.

Summarize the results to the maintainer before moving to another machine. If the candidate commit or
wheel checksum changes afterward, mark this evidence stale and repeat the complete procedure with
the regenerated artifact.
