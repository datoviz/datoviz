# Build From Source

Run build and test commands from the repository root.


## Primary Workflow

```bash
just clean
just build
just test
```

For focused work, run the narrowest relevant test filter:

```bash
just test scene
just test drp2
```


## Graphics Tests

Some Vulkan, GLFW, and platform-window tests need the repository runtime environment. On systems
that use `direnv`, run graphics-path tests through:

```bash
direnv exec . just test scene
```


## Documentation Edits

For documentation-only changes, run:

```bash
git diff --check
git status --short
```
