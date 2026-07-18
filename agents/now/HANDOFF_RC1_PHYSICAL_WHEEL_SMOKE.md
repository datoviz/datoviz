# RC1 Physical Validation Dispatch

Status: active physical-machine validation. Updated: 2026-07-18.

For `0.4.0rc1`, execute the reusable
[physical release-validation procedure](../../docs/contributors/release-physical-validation.md) on
each available required Linux, macOS, and Windows machine. Durable acceptance policy lives in
[`spec/release/PHYSICAL_VALIDATION.md`](../../spec/release/PHYSICAL_VALIDATION.md).

Accept only wheels from an all-green `wheels.yml` run whose `headSha` equals the final RC1 candidate
commit. Record the run ID and wheel checksum on every machine.

A problem found on the Windows laptop has been fixed locally but is not yet part of regenerated CI
wheels. Current Windows evidence is provisional. After explicit approval to push the fix, wait for
an all-green exact-SHA workflow, download the regenerated wheels, and repeat the complete physical
procedure on all required machines. The new commit and checksums invalidate earlier Linux, macOS,
and Windows passes.

Do not push, dispatch the workflow, upload evidence, or publish from this handoff without explicit
approval in the current session.
