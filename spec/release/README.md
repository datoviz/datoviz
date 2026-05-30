# Release Spec Index

This directory owns durable v0.4 release policy. It is not an execution queue.

Use `agents/now/RELEASE.md` for the current route to the next release candidate. Use this directory
for release contracts that should survive agent handoffs, documentation rebuilds, and RC planning.


## Documents

- [READINESS.md](READINESS.md): feature/API freeze expectations, quality audits, packaging,
  legal/asset checks, and release-blocking validation.
- [RC_PROCESS.md](RC_PROCESS.md): RC1, RC2, RC3, and final-release gates and required artifacts.
- [COMMUNICATION.md](COMMUNICATION.md): release notes, blog/announcement assets, public messages,
  and feedback channels.
- [GALLERY_OUTREACH.md](GALLERY_OUTREACH.md): real-dataset showcase and scientist outreach policy.


## Source-Of-Truth Boundaries

1. Active sequencing and current blockers stay in `agents/now/`.
2. Public documentation structure stays in [`../docs/`](../docs/).
3. Example coverage and metadata stay in
   [`../docs/EXAMPLE_COVERAGE.md`](../docs/EXAMPLE_COVERAGE.md) and
   [`../scene/examples/`](../scene/examples/).
4. Dataset layout, provenance, promotion, and submodule policy stay in
   [`../data/V0_4_DATA_REPOSITORY.md`](../data/V0_4_DATA_REPOSITORY.md).
5. Scene, DRP2, API, and binding behavior contracts stay in their owning `spec/` directories.

Release specs may link to those sources, but should not duplicate detailed API, example, or data
mechanics.
