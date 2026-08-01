# AGENTS.md - Datoviz v0.4-dev Agent Guide

Mandatory entry point for automation agents in this repository. Keep this file short. Detailed
rules live in [agents/rules/](agents/rules/).

Datoviz v0.4-dev is a deep rewrite of v0.3 and is now the active v0.4 release-candidate line.


## Non-Negotiable Rules

1. Do not preserve v0.3 compatibility at the expense of v0.4 architecture, correctness, or
   maintainability.
2. Do not stage, commit, or push changes inside the `data` submodule unless the user explicitly
   approves that submodule commit or pointer update in the current turn.
3. Treat staged `data` gitlink updates as stop signs until explicitly approved. Unstaged or
   untracked `data` working-tree state may be ignored only if it remains unstaged and uncommitted.
4. Do not stage or commit generated/runtime binary payloads such as `libs/vulkan/`, `*.dylib`,
   `*.so`, `*.dll`, `*.npy`, `*.npz`, or `.DS_Store` unless the user explicitly approves those
   exact files in the current turn.
5. Always run `git diff --check` before finalizing code changes.
6. Before committing, run `git status --short` and `git diff --cached --stat`; verify the staged set
   excludes unapproved `data`, generated files, vendored runtime libraries, large binaries, and
   unrelated user changes.
7. After changing public headers, exported API, binding policy, or binding generators, refresh and
   validate local Python bindings with `just ctypes` and `just ctypes-check` before running Python,
   GSP, or packaging validation.
8. Pushing commits to `origin/v0.4-dev` is allowed when the user explicitly requests a push in the
   current turn. All other external publication requires the user's explicit manual approval of the
   exact final content and publication action first. This includes GitHub comments, reviews, issues,
   pull requests, releases, pushes to other branches, messages, uploads, and posts. Prepare drafts
   only until that approval is given; ambiguous instructions such as "do it" do not authorize
   publication.
   Use the GitHub connector for read-only repository, issue, and pull-request inspection. Use the authenticated `gh` CLI for approved GitHub mutations so public actions appear directly under the maintainer's identity without connector attribution. If `gh` is unavailable, stop and obtain explicit approval before using the connector for a mutation.
9. Never hard-wrap Markdown prose, including list items. Keep each paragraph or list item on one source line; use line breaks only where Markdown structure requires them, such as headings, blank paragraph separators, separate list or table rows, blockquotes, and fenced code blocks.

More detail: [agents/rules/REPO_HYGIENE.md](agents/rules/REPO_HYGIENE.md).


## Operating Style

Write short, dense prose. Prefer concrete decisions over broad narration.
When you ask the user for a decision, always include my own preference with the ask.

For non-trivial implementation work, first propose a concise plan and wait for user approval. After
approval, execute end to end: inspect, edit, test, run `git diff --check`, and report results.

For long multi-stage work, make logical checkpoint commits after relevant checks pass. Never commit
unrelated user changes, stop-sign paths, generated binaries, or staged surprises.

Before implementing a narrow request, check whether a small generalization, cleaner module boundary,
or focused refactor would reduce duplication or future churn. Ask before broadening scope.

Keep files and folders modular. Split files that mix responsibilities, avoid large flat folders, and
prefer reusable subsystem boundaries over one-off helpers.


## Active Stack

Active modules built into `libdatoviz` by default:

`common`, `fileio`, `geom`, `math`, `thread`, `shader`, `input`, `window`, `canvas`, `stream`, `video`, `vk`, `vklite`, `drp2`, `scene`, and `app`.

The active runtime path is:

```text
scene frame plans -> drp2 command streams -> vklite runtime ->
canvas/stream frame execution -> optional app presentation
```

Treat `vk`, `vklite`, `canvas`, `stream`, `video`, and `window` as the runtime foundation. Do not
create parallel presentation, frame-stream, renderer, or Vulkan-wrapper paths unless explicitly
asked.


## Start Here

1. Read [agents/now/START.md](agents/now/START.md) for current branch dispatch.
2. Read [agents/now/STATUS.md](agents/now/STATUS.md) for active release blockers.
3. Read [agents/now/RELEASE.md](agents/now/RELEASE.md) for release sequencing.
4. Read [spec/scene/README.md](spec/scene/README.md) before changing scene semantics or runtime
   boundaries.


## Build, Test, Code

Run from the repository root:

```sh
just build
just test [filter]
just ctypes-check   # after public API/header/binding changes
git diff --check
```

Use the narrowest relevant validation loop while iterating. For Vulkan/GLFW/Metal paths on macOS,
prefer:

```sh
direnv exec . just test [filter]
```

Public headers live in `include/datoviz/`. Internal implementation lives in `src/`. Shared internal
helpers live in `src/common`.

Follow the project C style in touched files: Doxygen docstrings for new module-level functions,
three blank lines between neighboring top-level definitions, `dvz_` for public symbols, project
allocation/copy/I/O helpers, no file-scope mutable state, and focused tests for lifetime, bounds, or
cross-module contract changes.

For graphics work, make Vulkan ownership explicit. Do not destroy, begin, end, reset, submit, or
transition borrowed handles unless the API contract grants that ownership.

More detail:

1. [agents/rules/BUILD_TEST.md](agents/rules/BUILD_TEST.md)
2. [agents/rules/C_CODING.md](agents/rules/C_CODING.md)
3. [agents/rules/GRAPHICS_SAFETY.md](agents/rules/GRAPHICS_SAFETY.md)
4. [agents/rules/SCENE_DRP2.md](agents/rules/SCENE_DRP2.md)


## Documentation Routing

Use `spec/` for durable v0.4 design, semantics, and architecture. Use `agents/` only for active
execution status, handoff notes, and repo-wide agent rules. The public `docs/` tree may be rebuilt
in place for v0.4 user documentation; do not put private implementation plans there.
