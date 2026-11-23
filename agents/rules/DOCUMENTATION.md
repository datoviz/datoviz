# Documentation Validation

These rules apply to public documentation, gallery, attribution, and release communication changes. Instruction-only edits follow [build and validation](BUILD_TEST.md#validation-strategy).

## Prose Review

All new or revised documentation prose must go through the `humanizer` skill before it is considered finished. This covers the entire course, guides, reference explanations, contributor documentation, specifications, README files, release notes and agent documentation. Read and apply the skill's `SKILL.md`; a generic readability review or asking for a natural tone does not count as using the skill.

Write simple, clear, direct prose, with no more words than the explanation needs. Remove filler, slogans, stock transitions, repeated caveats and forced contrasts. Keep the reasoning, examples and technical qualifications readers need; brevity must not make a lesson cryptic. Preserve facts, uncertainty, API names, code, commands, numbers and link targets.

When delegating writing, explicitly require the writer to read and apply the humanizer skill. Review the result for both technical accuracy and natural wording. If the skill is unavailable, report the missing pass and leave prose review pending. Record the files or sections actually reviewed in the task handoff; do not imply that untouched prose or the whole course passed because one section did.

Existing prose is not certified by this rule. Apply the pass as pages are revised, and track any requested broader review separately. Edit authored sources rather than generated output.

## Validation

Documentation-only changes require `git diff --check`, `just docs-build-check`, `just docs-status-check`, and inspection of `git status --short`. Run the recipes with their declared build dependencies. Generated references, examples, screenshots, animations, or inventories additionally require their focused generator and checker.

Use [adding examples](../../docs/contributors/adding-examples.md) for public example contributions and [gallery media](../../docs/contributors/gallery-media.md) for capture, encoding, and media-cache changes. Follow [repository hygiene](REPO_HYGIENE.md#documentation-images) for image size, attribution, and protected assets.

Keep Markdown paragraphs and list items on one source line. In code examples, put explanatory comments on their own line above the code they describe.

For release scope, readiness, or publication preparation, also read the [documentation inventory](../now/DOCUMENTATION.md). Existing publication and protected-path approval requirements in [AGENTS.md](../../AGENTS.md#required-boundaries) apply throughout.

## Examples And Figures

- On pages offering Python and C, put every Python/C code example in a complete pair of language tabs, with Python first and equivalent operations. Do not leave later snippets outside the tabs or provide an empty alternative. Shared shell commands, shaders and data formats may remain outside the host-language choice.
- A language choice applies across the whole page, including closed sections, and persists between pages, reloads and back/forward navigation. Explicitly single-language pages keep their own language.
- Use captioned Mermaid or SVG figures for conceptual diagrams. Keep literal terminal output and file paths in monospace. Preserve readable labels on phones; let wide diagrams scroll instead of shrinking their text.
- Reuse the selected, approved video and its codec fallbacks wherever the same example appears. Avoid separate page-specific versions of the same demonstration.

Run `just doc-languages-check` for source structure and `just docs-build-check` for the rendered HTML gate. After changing the language switcher or adding mixed-language examples, also run `just doc-languages-browser-check`; it checks every mixed page in the built site, persistence, hidden sections and browser-storage fallback. The Linux documentation CI runs these checks. Install Playwright Chromium first, or pass an available browser with `--browser-executable`.

## Ongoing Review Workflow

Use the managed private gateway workspace for ongoing documentation review. Its WIP documentation, campaign dashboard, and temporary visual reviews are separate destinations; do not use the campaign dashboard as a documentation server or launch ad hoc HTTP/TLS review ports.

The WIP preview watches documentation, configuration, and tutorial sources, builds prerequisites, and publishes only successful stable-input builds. A failed build leaves the last good preview available and reports its status. After a successful build, refresh an open page. This convenience preview does not replace the required documentation checks or frozen-candidate evidence.

Temporary visual reviews use dated, hash-identified candidates with rationale and provenance. Viewing a page is not approval and never promotes canonical media. Private service configuration and activation belong to the gateway runtime, outside the Datoviz tree; keep machine-access details out of tracked instructions.
