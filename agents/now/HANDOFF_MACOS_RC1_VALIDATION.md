# macOS RC1 Physical Validation Dispatch

Status: active RC1 machine dispatch. Updated: 2026-07-18.

For an Apple Silicon or Intel Mac, follow the current
[RC1 physical-validation dispatch](HANDOFF_RC1_PHYSICAL_WHEEL_SMOKE.md) and the reusable
[physical release-validation procedure](../../docs/contributors/release-physical-validation.md).

For RC1, Apple Silicon runs the `full` installed-wheel profile and complete live interaction set.
Intel runs at least the `rc` installed-wheel profile and focused live set. Apple Silicon also runs
the IPython close/reopen lifecycle check; Qt/PyQt remains an optional-provider pass or explicit
skip.

For the current candidate, use run `29622780390` at artifact commit `bfa569916` if it passes. The
descendant release commit `73b10cf95` contains only audited release-process documentation, so it
does not invalidate the Mac artifact or manual pass. Apply the general intervening-diff audit if the
release commit advances again.
