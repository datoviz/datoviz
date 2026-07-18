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

Do not accept run #457 as macOS physical installed-wheel evidence. On an M3 MacBook Air, the full
validator passes only when it inherits the checkout's Vulkan SDK environment. A fresh wheel install
without those variables cannot initialize Volk because the wheel lacks `MoltenVK_icd.json` and the
`libvulkan.dylib` / `libvulkan.1.dylib` loader names that Volk searches. The packaged versioned
Vulkan loader and MoltenVK dylibs render successfully when those discovery inputs are supplied,
which isolates the blocker to macOS wheel runtime packaging/discovery. Fix it, add a no-SDK render
smoke, rerun the wheel workflow, and use the corrected checksum for physical validation.

If RC1 advances again, audit the intervening diff. Regenerate wheels and repeat affected
physical evidence only if it changes artifact/runtime inputs, changes a wheel checksum, or cannot
be classified confidently as neutral.

Do not push, dispatch the workflow, upload evidence, or publish from this handoff without explicit
approval in the current session.
