# Python Binding Exact-Call Policy

Status: durable v0.4 binding policy.

This note records how to treat `datoviz.raw`, the exact C-shaped call form of the generated Python
`ctypes` binding. The source of truth for current generator policy is `spec/bindings/ctypes.yml`.


## Layering

`datoviz.raw` is:

- exact ABI access for generated `ctypes`;
- validation, debugging, and advanced FFI work;
- the implementation substrate under the top-level NumPy-adapted call form.

It is not a separate high-level API and is not the high-level plotting API. The recommended
direct-engine Python entry point is:

```python
import datoviz as dvz
```

GSP/VisPy2 owns high-level plotting, Pythonic scene objects, and object-oriented scientific UX.


## Skipped Functions

Do not bind skipped functions merely to reduce the skipped count. A `datoviz.raw` entry point is
appropriate only when the signature is useful, layout-stable, and safe enough for exact `ctypes`
callers.

The current skipped-function list is in `spec/bindings/ctypes.yml` under
`skipped_functions.expected`. Keep it synchronized with `datoviz._ctypes._SKIPPED_FUNCTIONS`.

Each skipped function must have a disposition in `skipped_functions.dispositions`:

- `emit`: add the record to `layout_records.include` only when the full ABI layout is stable and
  user-facing.
- `ffi-helper`: expose a narrowly named `dvz_ffi_*` helper when the canonical C function is useful
  from `datoviz.raw` but unsafe or awkward as a by-value return.
- `intentional-skip`: keep skipped and document why.
- `remove-export`: remove or demote the C export if the function is accidental public API.

Prefer `intentional-skip` for low-level Vulkan/runtime configuration and swapchain/surface
by-value helpers unless a concrete supported Python workflow requires a safe binding shape.

Descriptor/default helpers may use pointer-output `dvz_ffi_*` wrappers when that avoids by-value
layout hazards while preserving a useful exact-call Python workflow.


## Validation

After changing public headers, exported symbols, binding policy, or binding generators, run:

```sh
just ctypes
just ctypes-check
just ctypes-smoke
just docs-api-check
git diff --check
```

If unrelated dirty public headers would affect generated output, do not regenerate over them for an
unrelated policy commit. Isolate the change or clearly report the blocker.
