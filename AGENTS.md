# Datoviz Agent Guide

Shared instructions for coding agents. Claude Code loads this file through [CLAUDE.md](CLAUDE.md).

Datoviz v0.4 is a deep rewrite of v0.3. The active development and release-candidate branch is `main`.

## Required Boundaries

1. Do not preserve v0.3 compatibility at the expense of v0.4 architecture, correctness, or maintainability.
2. Do not stage, commit, or push changes inside the `data` submodule unless the user explicitly approves that submodule commit or pointer update in the current turn.
3. Treat staged `data` gitlink updates as stop signs until explicitly approved. Unstaged or untracked `data` working-tree state may be ignored only if it remains unstaged and uncommitted.
4. Before committing or pushing a `data` gitlink, verify that its exact commit is already reachable from the advertised remote branch recorded in `.gitmodules`; publish the `data` commit first, then the parent gitlink.
5. Do not stage or commit generated/runtime binary payloads such as `libs/vulkan/`, `*.dylib`, `*.so`, `*.dll`, `*.npy`, `*.npz`, or `.DS_Store` unless the user explicitly approves those exact files in the current turn.
6. Always run `git diff --check` before finalizing code changes.
7. Before committing, run `git status --short` and `git diff --cached --stat`; verify the staged set excludes unapproved `data`, generated files, vendored runtime libraries, large binaries, and unrelated user changes.
8. After changing public headers, exported API, binding policy, or binding generators, refresh and validate local Python bindings with `just ctypes` and `just ctypes-check` before running Python, GSP, or packaging validation.
9. Pushing commits to `origin/main` is allowed when the user explicitly requests a push in the current turn. All other external publication requires the user's explicit manual approval of the exact final content and publication action first. This includes GitHub comments, reviews, issues, pull requests, releases, pushes to other branches, messages, uploads, and posts. Prepare drafts only until that approval is given; ambiguous instructions such as "do it" do not authorize publication. Use the GitHub connector for read-only repository, issue, and pull-request inspection. Use the authenticated `gh` CLI for approved GitHub mutations so public actions appear directly under the maintainer's identity without connector attribution. If `gh` is unavailable, stop and obtain explicit approval before using the connector for a mutation.
10. Never hard-wrap Markdown prose, including list items. Keep each paragraph or list item on one source line; use line breaks only where Markdown structure requires them, such as headings, blank paragraph separators, separate list or table rows, blockquotes, and fenced code blocks.

Read [repository hygiene](agents/rules/REPO_HYGIENE.md) before staging, committing, pushing, or changing repository structure.

## Working Style

For implementation requests with clear scope, state a concise plan and execute through relevant validation. Resolve routine implementation choices independently. Ask before materially broadening scope or when an unresolved decision changes the intended result. If the user asks for a plan or review, provide that without implementing it.

Continue within an approved plan without requesting approval again for routine steps. The publication, submodule, and binary-asset approval requirements above still apply. If an instruction blocks requested work, identify the file and rule, explain the blocker, and give a recommended next step.

Keep changes focused and files modular. Reuse existing subsystem boundaries; ask before expanding a narrow task into a broader refactor.

Write short, concrete prose. When asking for a decision, include your recommendation. Report the result, relevant validation, and any remaining limitation.

For long implementation tasks, make logical checkpoint commits after relevant checks pass. Group related changes by coherent result; never commit unrelated user changes or unapproved paths.

## Architecture

```text
scene frame plans -> drp2 command streams -> vklite runtime ->
canvas/stream frame execution -> optional app presentation
```

Treat `vk`, `vklite`, `canvas`, `stream`, `video`, and `window` as the runtime foundation. Do not create parallel presentation, frame-stream, renderer, or Vulkan-wrapper paths unless explicitly asked.

For graphics work, make Vulkan ownership explicit. Do not destroy, begin, end, reset, submit, or transition borrowed handles unless the API contract grants that ownership.

## Read For The Task

Read the matching rules before editing; follow their more specific routes only when relevant. Ordinary fixes do not require reading release history.

| Task | Read |
| --- | --- |
| C/C++ implementation | [C conventions](agents/rules/C_CODING.md), [build and validation](agents/rules/BUILD_TEST.md) |
| Build, public headers, bindings, or tests | [Build and validation](agents/rules/BUILD_TEST.md) |
| Graphics ownership, rendering, or frame execution | [Graphics safety](agents/rules/GRAPHICS_SAFETY.md) |
| Scene semantics, visuals, shaders, or DRP2 | [Scene/DRP2 rules](agents/rules/SCENE_DRP2.md), [scene specs](spec/scene/README.md) |
| Documents under `spec/scene/` | [Scene spec writing rules](spec/scene/AGENTS.md) |
| Public examples, screenshots, WebGPU routes, animation, or video | [Adding examples](docs/contributors/adding-examples.md) |
| Release-sensitive work, active handoffs, or choosing the next task | [Dispatch](agents/now/START.md), then only the relevant lane |
| Public documentation, gallery, attribution, or release communication | [Documentation gates](agents/now/DOCUMENTATION.md) |

## Validation

Run from the repository root:

```sh
just build
just test <filter>
git diff --check
```

For Vulkan/GLFW/Metal tests on macOS, use `direnv exec . just test <filter>`. After public API or binding changes, run `just ctypes` and `just ctypes-check` before Python-facing validation.

Use the narrowest relevant checks. Instruction-only edits need link/content checks and `git diff --check`. Public documentation also requires the checks in `agents/now/DOCUMENTATION.md`, including any recipe build dependencies. Complete required checks, then broaden or repeat testing only when changes, failures, or unresolved risks justify it. Report unavailable validation as a limitation, never a pass.

## Documentation

Use `spec/` for durable design, semantics, and architecture; `agents/now/` for current execution status and handoffs; `agents/rules/` for shared agent rules; and `docs/` for public user and contributor documentation. Do not put private implementation plans in `docs/`.

In documentation code examples, put explanatory comments on their own line above the code they describe.
