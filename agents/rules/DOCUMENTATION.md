# Documentation Validation

These rules apply to public documentation, gallery, attribution, and release communication changes. Instruction-only edits follow [build and validation](BUILD_TEST.md#validation-strategy).

Documentation-only changes require `git diff --check`, `just docs-build-check`, `just docs-status-check`, and inspection of `git status --short`. Run the recipes with their declared build dependencies. Generated references, examples, screenshots, animations, or inventories additionally require their focused generator and checker.

Use [adding examples](../../docs/contributors/adding-examples.md) for public example contributions and [gallery media](../../docs/contributors/gallery-media.md) for capture, encoding, and media-cache changes. Follow [repository hygiene](REPO_HYGIENE.md#documentation-images) for image size, attribution, and protected assets.

Keep Markdown paragraphs and list items on one source line. In code examples, put explanatory comments on their own line above the code they describe.

For release scope, readiness, or publication preparation, also read the [documentation inventory](../now/DOCUMENTATION.md). Existing publication and protected-path approval requirements in [AGENTS.md](../../AGENTS.md#required-boundaries) apply throughout.
