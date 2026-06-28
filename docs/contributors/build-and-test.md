# Build and Test

Run build and test commands from the repository root.


## Primary Workflow

```bash
just build
just test
```

`just build` uses Ninja on Linux and macOS. Ninja chooses a CPU-based job count by default. To pin
the build fan-out on a busy developer machine, set:

```bash
DVZ_BUILD_JOBS=8 just build
```

When `ccache` is available, the build recipes use it for both C and C++ compilation. Override or
disable the compiler launcher with:

```bash
DVZ_COMPILER_LAUNCHER=sccache just build
DVZ_COMPILER_LAUNCHER=OFF just build
```

For a quick build-graph/cache snapshot:

```bash
just build-stats
```

Use `just clean` only when you need to discard local build artifacts or reset a stale configuration.

For focused work, run the narrowest relevant test filter:

```bash
just test scene
just test drp2
just spec-check
```


## Graphics Tests

Some Vulkan, GLFW, and platform-window tests need the repository runtime environment. On systems
that use `direnv`, run graphics-path tests through:

```bash
direnv exec . just test scene
```


## Documentation Edits

For documentation-only edits:

```bash
git diff --check
git status --short
```

Before committing, inspect the staged set:

```bash
git diff --cached --stat
```

Do not stage unrelated work, `data` submodule pointer updates, generated binary payloads, or
vendored runtime libraries unless explicitly approved for the current change.
