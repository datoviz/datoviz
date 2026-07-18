# macOS RC1 Physical Validation Dispatch

Status: active RC1 machine dispatch. Updated: 2026-07-18.

For an Apple Silicon or Intel Mac, follow the current
[RC1 physical-validation dispatch](HANDOFF_RC1_PHYSICAL_WHEEL_SMOKE.md) and the reusable
[physical release-validation procedure](../../docs/contributors/release-physical-validation.md).

For RC1, Apple Silicon runs the `full` installed-wheel profile and complete live interaction set.
Intel runs at least the `rc` installed-wheel profile and focused live set. Apple Silicon also runs
the IPython close/reopen lifecycle check; Qt/PyQt remains an optional-provider pass or explicit
skip.

Run `29624999442` (`Wheels` #457) passed at artifact commit `06317fa4f`, but its macOS wheels are not
eligible for physical installed-wheel acceptance. The arm64 artifact passes under this machine's
Vulkan SDK environment and its native binaries are architecture-correct, but a fresh install cannot
render without external Vulkan discovery variables: the wheel omits `MoltenVK_icd.json` and the
loader aliases Volk searches on macOS. Fix the packaging/runtime discovery path, require a no-SDK
installed render smoke in CI, rerun the matrix, and perform the Quickstart plus live interaction set
only on the corrected checksum.
