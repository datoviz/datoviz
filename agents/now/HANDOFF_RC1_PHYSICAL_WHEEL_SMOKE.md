# RC1 Physical Validation Dispatch

Status: active physical-machine validation. Updated: 2026-07-18.

For `0.4.0rc1`, execute the reusable
[physical release-validation procedure](../../docs/contributors/release-physical-validation.md) on
each available required Linux, macOS, and Windows machine. Durable acceptance policy lives in
[`spec/release/PHYSICAL_VALIDATION.md`](../../spec/release/PHYSICAL_VALIDATION.md).

Wheel run
[29624999442](https://github.com/datoviz/datoviz/actions/runs/29624999442) (`Wheels` #457) passed at
artifact commit `06317fa4f` (`fix: harden wheel build and CI validation`). The macOS arm64 artifact
has SHA-256 `b133379c828ad0bca7554d8df321a047813052cccdd3af1d955728c9e8f92118`.

Run #457 is superseded for RC evidence. Its Windows AMD64 wheel passed physical end-to-end
validation. Its Linux x86_64 wheel also passed functional validation, but the installed native
version reported `0.4.0rc1 (DEBUG)`, so it is not a release artifact. The standalone macOS
MoltenVK/ICD packaging fix landed in `30bbbe483`. Use only artifacts from the replacement matrix
that enforces a Release native build; record its run, commit, and per-wheel checksums here.

Run
[29634509734](https://github.com/datoviz/datoviz/actions/runs/29634509734) (`Wheels` #459) at
`77d4eeebf` produced Release wheels. Its Linux x86_64 wheel has SHA-256
`ffcca2c4f299e3f677c15df1d85ab198628a98b86225c42188b34169f964bacb`; the full installed-wheel
validator and physical Quickstart resize, pan, zoom, and close checks passed. The Windows AMD64
wheel also passed physical end-to-end validation. Do not promote run #459 as the final matrix: its
macOS Intel render smoke ran on a hosted VM without GPU/Metal acceleration and aborted. Commit
`7ceb18df5` gates hosted Intel validation to non-render checks, leaving Intel render proof to the
physical machine. The job-local CMake arguments also overrode the workflow default and re-enabled
mimalloc; the replacement run must retain `DVZ_MIMALLOC_SOURCE=OFF` on macOS.

Do not accept run #457 as macOS or Linux physical installed-wheel evidence. On an M3 MacBook Air, the full
validator passes only when it inherits the checkout's Vulkan SDK environment. A fresh wheel install
without those variables cannot initialize Volk because the wheel lacks `MoltenVK_icd.json` and the
`libvulkan.dylib` / `libvulkan.1.dylib` loader names that Volk searches. The packaged versioned
Vulkan loader and MoltenVK dylibs render successfully when those discovery inputs are supplied,
which isolated the blocker to macOS wheel runtime packaging/discovery. The fix packages the loader
aliases, MoltenVK, and sibling ICD manifest and adds a no-SDK render smoke. Rerun the wheel workflow
and use the corrected checksums for physical validation.

If RC1 advances again, audit the intervening diff. Regenerate wheels and repeat affected
physical evidence only if it changes artifact/runtime inputs, changes a wheel checksum, or cannot
be classified confidently as neutral.

Do not push, dispatch the workflow, upload evidence, or publish from this handoff without explicit
approval in the current session.
