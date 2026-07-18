# Release Validation Matrix

This matrix defines what release automation should ask the maintainer to validate. It is a target
for the v0.4 automation, not a claim that all runners are already implemented.


## Profiles

| Profile | Purpose | Typical runtime |
| --- | --- | --- |
| `quick` | Import, ABI, C consumer, and one render smoke from installed artifacts. | Minutes |
| `rc` | Required release-candidate validation for a supported machine. | Tens of minutes |
| `full` | Broader examples, docs snippets, Vulkan validation, and selected live smoke. | Longer/manual |
| `manual` | Human-guided live examples and interaction checks. | Maintainer-driven |


## Required Machine Classes

The exact machine names are maintainer-local. Each class should have one stable `machine_id` in the
evidence bundle.

| Class | Required for RC | Required for final | Main proof |
| --- | --- | --- | --- |
| macOS Apple Silicon | yes | yes | arm64 wheel install, offscreen render, live app smoke, optional Qt/PyQt |
| macOS Intel | yes if available | yes if available | x86_64 wheel install, native dependency inventory, import/render smoke |
| Linux x86_64 with Vulkan GPU | yes | yes | manylinux wheel install, Vulkan validation, C/Python examples |
| Linux aarch64 | artifact required, execution if available | artifact required, execution if available | wheel inventory and native execution when a host exists |
| Windows AMD64 | yes | yes | wheel install, `datoviz.raw`, CMake consumer, Python smoke |
| Windows ARM64 | artifact required, execution if available | artifact required, execution if available | wheel inventory and native execution when a host exists |


## RC Profile Checks

The `rc` profile should run from installed artifacts where possible:

1. install wheel or source bundle into a clean environment;
2. import `datoviz` and `datoviz.raw`;
3. run ABI/layout smoke checks;
4. run `datoviz-config` or `python -m datoviz.cli --cflags --libs --cmake-dir`;
5. compile and run the installed CMake consumer;
6. run at least one offscreen render smoke;
7. run representative C examples from release metadata;
8. run representative Python examples from release metadata;
9. capture stdout/stderr and Vulkan validation output when available;
10. record skipped optional providers, such as Qt/PyQt, with explicit reasons.


## Full Profile Additions

The `rc` profile includes wheel inventory, installed import/CLI checks, CMake consumer proof, and
installed Python/C no-window example smokes from the candidate wheel. It should run on required
machine classes before accepting the RC.

The `full` profile may add:

1. installed Python/C offscreen render example smokes from the candidate wheel;
2. all public C examples that support bounded `--png` or `--smoke-ms` execution;
3. all public Python examples classified as automated or smokeable;
4. documentation fenced-code doctests;
5. gallery capture and media fingerprint checks;
6. WebGPU/WASM browser smoke on capable hosts;
7. scripted interaction checks for panzoom, arcball, resize, picking, and close-window behavior;
8. longer live-loop churn tests;
9. Vulkan validation-layer runs for runtime ownership and synchronization paths.


## Manual Profile

The manual profile is intentionally short and targeted. Durable requirements live in
[PHYSICAL_VALIDATION.md](PHYSICAL_VALIDATION.md), and the reusable agent procedure lives in
[`docs/contributors/release-physical-validation.md`](../../docs/contributors/release-physical-validation.md).
The local agent must launch the selected apps one at a time and ask the maintainer to perform the
stated interaction; it must not hand-maintain an ad hoc checklist.

Manual smoke should cover:

1. one simple 2D live app with panzoom;
2. one 3D live app with arcball or fly controls;
3. one text/annotation/layout example;
4. one image or color-scale example;
5. one mesh or textured mesh example;
6. one picking/query or readback example when supported on that platform;
7. resize, close-window, and repeated-open behavior.


## Evidence Policy

For each machine class, the report should show:

1. artifact installed;
2. profile run;
3. pass/fail/skip status;
4. GPU/driver/runtime facts;
5. known issues or exclusions;
6. capture artifacts when available.

The release report must not infer support from an artifact existing. A wheel build proves inventory;
execution proof comes from a native or otherwise trusted runtime evidence bundle.
