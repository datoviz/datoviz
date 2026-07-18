# macOS RC1 Physical Validation Dispatch

Status: active RC1 machine dispatch. Updated: 2026-07-18.

For an Apple Silicon or Intel Mac, follow the current
[RC1 physical-validation dispatch](HANDOFF_RC1_PHYSICAL_WHEEL_SMOKE.md) and the reusable
[physical release-validation procedure](../../docs/contributors/release-physical-validation.md).

For RC1, Apple Silicon runs the `full` installed-wheel profile and complete live interaction set.
Intel runs at least the `rc` installed-wheel profile and focused live set. Apple Silicon also runs
the IPython close/reopen lifecycle check; Qt/PyQt remains an optional-provider pass or explicit
skip.

Any successful Mac result obtained before the pending Windows fix is included in a new exact-SHA
wheel workflow becomes stale when that candidate commit or wheel checksum changes. Repeat it with
the regenerated Mac wheel before accepting the final RC1 matrix.
