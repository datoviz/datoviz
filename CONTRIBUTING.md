# Contributing

Datoviz v0.4 is a deep rewrite of the v0.3 codebase. Contributions should follow the v0.4
architecture and current release-candidate priorities, not preserve old APIs at the expense of the
new runtime path.

Use this file as the repository entry point. The detailed contributor manual lives in
[`docs/contributors/`](docs/contributors/).


## Start Here

Before changing code, read:

1. [`AGENTS.md`](AGENTS.md) for repository guardrails and active module boundaries;
2. [`agents/now/START.md`](agents/now/START.md) for current branch dispatch;
3. [`agents/now/STATUS.md`](agents/now/STATUS.md) for active release blockers;
4. the relevant durable spec under [`spec/`](spec/) before changing behavior or public contracts.

For normal contributor workflows:

| Need | Read |
| --- | --- |
| Build and test commands | [`docs/contributors/build-and-test.md`](docs/contributors/build-and-test.md) |
| C style, generated files, and validation expectations | [`docs/contributors/coding-style.md`](docs/contributors/coding-style.md) |
| Public examples, screenshots, WebGPU, and video | [`docs/contributors/adding-examples.md`](docs/contributors/adding-examples.md) |
| Visual families | [`docs/contributors/adding-a-visual.md`](docs/contributors/adding-a-visual.md) |
| Documentation pages | [`docs/contributors/docs-authoring.md`](docs/contributors/docs-authoring.md) |
| Coding-agent workflows | [`docs/contributors/ai-agents.md`](docs/contributors/ai-agents.md) |
| Release-candidate work | [`docs/contributors/release-process.md`](docs/contributors/release-process.md) |


## Default Commands

Run commands from the repository root:

```sh
just build
just test
git diff --check
```

Use the narrowest relevant validation loop while iterating. For documentation-only edits:

```sh
git diff --check
git status --short
```


## Hygiene Rules

Do not stage, commit, or push the `data` submodule pointer unless the maintainer explicitly approves
that exact update in the current turn.

Before committing or pushing an approved `data` pointer update, push the `data` commit to its configured remote branch and run `python3 tools/check_submodule_reachability.py data` from the parent repository. A locally available submodule object is not proof that a fresh clone can obtain it.

Do not stage generated/runtime binary payloads such as `libs/vulkan/`, `*.dylib`, `*.so`, `*.dll`,
`*.npy`, `*.npz`, or `.DS_Store` unless the maintainer explicitly approves those exact files.

Before committing, inspect:

```sh
git status --short
git diff --cached --stat
```

The staged set must exclude unrelated user changes, unapproved `data` updates, generated binaries,
vendored runtime libraries, and stale generated files.
