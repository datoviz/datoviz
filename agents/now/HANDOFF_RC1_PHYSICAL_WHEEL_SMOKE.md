# RC1 Physical Validation Dispatch

Status: active physical-machine validation. Updated: 2026-07-18.

For `0.4.0rc1`, execute the reusable
[physical release-validation procedure](../../docs/contributors/release-physical-validation.md) on
each available required Linux, macOS, and Windows machine. Durable acceptance policy lives in
[`spec/release/PHYSICAL_VALIDATION.md`](../../spec/release/PHYSICAL_VALIDATION.md).

The current wheel run is
[29622780390](https://github.com/datoviz/datoviz/actions/runs/29622780390) at artifact commit
`bfa569916` (`fix: embed Windows wheel shader resources`). Accept it if all required jobs pass.
Record that artifact commit, the release commit, run ID, and wheel checksum on every machine.

The current release commit `73b10cf95` descends from the artifact commit through one
release-documentation/process change. The audited `bfa569916..73b10cf95` diff contains only
`agents/`, `docs/`, `spec/`, and `mkdocs.yml` instruction/navigation changes; it does not affect
wheel inputs, runtime behavior, or live example sources. Under the durable policy, this diff does
not require cancelling or rerunning the wheel workflow.

If RC1 advances again, audit the new intervening diff. Regenerate wheels and repeat affected
physical evidence only if it changes artifact/runtime inputs, changes a wheel checksum, or cannot
be classified confidently as neutral.

Do not push, dispatch the workflow, upload evidence, or publish from this handoff without explicit
approval in the current session.
