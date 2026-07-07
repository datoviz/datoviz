# Coding Style

This page summarizes the contributor-facing coding rules. The detailed automation rules are in
`agents/rules/`; when they are stricter, follow them.


## C And C++ Code

Use the existing C style in touched files:

1. public symbols use the `dvz_` prefix and live in `include/datoviz/`;
2. internal helpers stay module-local, `static`, or `_dvz_`-prefixed;
3. public headers are installed API; implementation belongs under `src/`;
4. new module-level functions need short Doxygen docstrings;
5. neighboring top-level definitions use three blank lines;
6. lines should stay near 100 characters when practical.

Use Datoviz helpers instead of raw C runtime calls:

| Need | Prefer |
| --- | --- |
| allocation | `dvz_calloc`, `dvz_malloc`, `dvz_free` |
| copying and clearing | `dvz_memcpy`, `dvz_memset` |
| assertions | `ANN()`, `ASSERT()` for internal invariants |
| formatted output | `dvz_snprintf`, `dvz_fprintf`, `dvz_vfprintf` |

Do not pass side-effectful expressions to `ANN()`, `ASSERT()`, or `DVZ_ASSUME()`. Evaluate once into
a local variable first.


## File Organization

Keep files modular. A source file should have one clear responsibility: public API implementation,
family-specific lowering, runtime execution, tests, or small shared helpers.

Use section banners in large C files when they improve navigation:

```c
/*************************************************************************************************/
/*  Section Name                                                                                 */
/*************************************************************************************************/
```

Omit empty sections. Avoid broad refactors while fixing a narrow bug unless the refactor is the
smallest safe route to remove duplication or clarify ownership.


## Ownership And Lifetime

Make ownership explicit in API contracts and implementation:

1. never destroy, reset, submit, transition, begin, or end borrowed Vulkan handles;
2. set destroyed pointers to `NULL` and Vulkan handles to `VK_NULL_HANDLE`;
3. keep destroy/free paths idempotent;
4. avoid retaining pointers into growable tables across calls that may reallocate;
5. return status or diagnostics for recoverable runtime failures.

For graphics work, read `agents/rules/GRAPHICS_SAFETY.md` before editing `vk`, `vklite`, `canvas`,
`stream`, `window`, `video`, `drp2`, `scene`, or `app`.


## Scene And Runtime Boundaries

The active runtime path is:

```text
scene frame plans -> drp2 command streams -> vklite runtime ->
canvas/stream frame execution -> optional app presentation
```

Do not create parallel renderers, presentation layers, frame streams, Vulkan wrappers, or
texture-name-based shortcuts. If a visual family needs behavior that generic render emission cannot
express, extend the normalized descriptor or lowering interface first.


## Generated Files

Generated files may be committed when they are part of the documented source tree, but do not edit
them by hand.

Important generated surfaces:

| File or tree | Regeneration path |
| --- | --- |
| `datoviz/_ctypes.py` | `just ctypes` |
| `datoviz/_array_facade.py` | `just ctypes` |
| `build/bindings/datoviz_api.json` | `just api-json` |
| example metadata JSON | `python3 tools/build_examples_manifest.py` |
| generated C API docs | documented docs build scripts |

Python binding policy lives in `spec/bindings/`. User-facing exact-call docs for `datoviz.raw` live
in `docs/reference/ctypes.md`.


## Python Code

Python support in v0.4 is low-level and binding-oriented. Datoviz has one generated `ctypes`
binding. Use:

1. top-level `import datoviz as dvz` for normal calls and policy-declared NumPy adaptation;
2. `datoviz.raw` for exact pointers, counts, bytes, callbacks, and ABI debugging;
3. `datoviz._ctypes` only as generated implementation detail.

Format Python with Ruff using the project `pyproject.toml` settings:

```sh
ruff format <paths>
ruff check <paths>
```


## Validation

Use the narrowest relevant loop while iterating, then run `git diff --check` before finalizing:

```sh
just build
just test <filter>
git diff --check
```

Add focused tests for lifetime, bounds, ownership, cross-module contracts, generated bindings, or
multi-frame behavior when your change affects those surfaces.
