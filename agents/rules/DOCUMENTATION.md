# Documentation Validation

These rules apply to public documentation, gallery, attribution, and release communication changes. Instruction-only edits follow [build and validation](BUILD_TEST.md#validation-strategy).

Documentation-only changes require `git diff --check`, `just docs-build-check`, `just docs-status-check`, and inspection of `git status --short`. Run the recipes with their declared build dependencies. Generated references, examples, screenshots, animations, or inventories additionally require their focused generator and checker.

Use [adding examples](../../docs/contributors/adding-examples.md) for public example contributions and [gallery media](../../docs/contributors/gallery-media.md) for capture, encoding, and media-cache changes. Follow [repository hygiene](REPO_HYGIENE.md#documentation-images) for image size, attribution, and protected assets.

Keep Markdown paragraphs and list items on one source line. In code examples, put explanatory comments on their own line above the code they describe.

For release scope, readiness, or publication preparation, also read the [documentation inventory](../now/DOCUMENTATION.md). Existing publication and protected-path approval requirements in [AGENTS.md](../../AGENTS.md#required-boundaries) apply throughout.

## Ongoing Review Workflow

Use the managed private gateway workspace for ongoing documentation review. Its WIP documentation, campaign dashboard, and temporary visual reviews are separate destinations; do not use the campaign dashboard as a documentation server or launch ad hoc HTTP/TLS review ports.

The WIP preview watches documentation, configuration, and tutorial sources, builds prerequisites, and publishes only successful stable-input builds. A failed build leaves the last good preview available and reports its status. After a successful build, refresh an open page. This convenience preview does not replace the required documentation checks or frozen-candidate evidence.

Temporary visual reviews use dated, hash-identified candidates with rationale and provenance. Viewing a page is not approval and never promotes canonical media. Private service configuration and activation belong to the gateway runtime, outside the Datoviz tree; keep machine-access details out of tracked instructions.
