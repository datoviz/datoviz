# Public API Pre-RC Cleanup Record

Status: completed and merged to `v0.4-dev`.

Purpose: keep a short active pointer to the July 2026 public API/ABI cleanup campaign without
leaving the full audit as active dispatch. The full pre-shrink audit remains in git history; use:

```sh
git show b75d8f36e:agents/now/HANDOFF_PUBLIC_API_PRE_RC_AUDIT.md
```


## Completed Campaign

Execution branch: `api/pre-rc-cleanup`.

Merged to `v0.4-dev`: `35d2e7fc4 Merge branch 'api/pre-rc-cleanup' into v0.4-dev`.

Branch close record: `6b2eff985 agents: close pre-rc cleanup dispatch`.

The campaign intentionally broke source/ABI where doing so improved the v0.4 public contract.
Completed themes:

1. removed unused, legacy, transitional, or accidental public APIs;
2. moved internal object/container plumbing out of the ordinary public surface;
3. collapsed old scene aliases and compatibility wrappers;
4. normalized public naming, spelling, and argument ordering;
5. tightened ownership, constness, pointer lifetimes, and raw `ctypes` owned-return policy;
6. made stable app-facing names backend-neutral, with GLFW-specific details treated as backend or
   interop surface;
7. classified DRP2/vklite protocol escape hatches as advanced/unstable where they remain public;
8. converted stable fallible mutators to `DvzResult` where appropriate;
9. flattened or split noisy nested public descriptors where the stable C/ABI/FFI surface benefited;
10. regenerated raw `ctypes`, generated C reference output, examples, and status/spec docs in the
    same checkpoint waves as public API breaks.


## Remaining RC1 Reconciliation

No separate API cleanup branch remains to merge. Before RC1, keep these surfaces reconciled with
the merged public API:

1. `include/datoviz/` installed headers;
2. `symbols.map` and exported `DVZ_EXPORT` ABI;
3. `spec/api/status.yml` and public status docs;
4. generated C reference pages under `docs/reference/c-api/`;
5. `datoviz/_ctypes.py`, `spec/bindings/ctypes.yml`, and raw binding docs;
6. top-level array-aware facade wrappers and smoke tests;
7. public C examples, gallery metadata, and WebGPU live-route examples;
8. GSP/VisPy2 boundary language in public docs and specs.

If any public header, exported API, binding policy, or binding generator changes again, follow the
repository rule:

```sh
just ctypes
just ctypes-check
git diff --check
```


## Guardrails For Future API Work

Use these defaults unless a newer spec explicitly supersedes them:

1. Do not preserve v0.3 compatibility at the expense of v0.4 correctness, architecture, or
   maintainability.
2. Prefer object-first argument ordering; selectors/config/ranges next; payload pointers before
   counts/sizes; byte-buffer APIs as `bytes, size_bytes`.
3. Use `DvzResult` for ordinary fallible Datoviz APIs, `bool` for predicates, raw integer returns
   for non-error counts/status values, and Vulkan-native result codes only in Vulkan escape hatches.
4. Keep `datoviz.raw` exact to the exported ABI, but avoid known raw-binding traps such as owned
   `char*` returns bound as `c_char_p`.
5. Do not add new descriptor-like public structs nested by value inside other public descriptors.
   Prefer minimal creation descriptors plus setters for secondary style, layout, formatting,
   placement, tick policy, or backend-specific configuration.
6. Keep stable app APIs backend-neutral. Put backend-specific helpers in backend or interop headers
   unless deliberately classified as stable ABI.
7. Move fixture, JSON/base64 convenience, raw fallback, and development diagnostics out of the
   stable public surface where practical; otherwise mark them advanced/unstable.
8. Migrate examples, tests, generated bindings, generated references, and public docs in the same
   checkpoint as each future public API break.


## Commit Trail

Useful campaign checkpoints:

1. `c53c9a019 docs: add public API pre-RC audit handoff`
2. `a16ce666f Document aggressive pre-RC API cleanup`
3. `5d865b45d Document nested descriptor API cleanup audit`
4. `35d2e7fc4 Merge branch 'api/pre-rc-cleanup' into v0.4-dev`
5. `6b2eff985 agents: close pre-rc cleanup dispatch`

Use `git log --oneline --grep='api:' --all` and
`git log --oneline --grep='agents: record' --all` to inspect the detailed checkpoint sequence.
