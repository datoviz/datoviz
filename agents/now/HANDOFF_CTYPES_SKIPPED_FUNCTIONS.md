# Raw ctypes skipped-function handoff

Status: active API-audit follow-up for v0.4 RC preparation.

Last relevant commit: `acdde47b1 Bind frame plan emit config in ctypes`.


## Current state

Raw `ctypes` generation is green with 21 skipped functions. `DvzFramePlanEmitConfig` is now a
validated layout record, so `dvz_frame_plan_emit_config()` is emitted directly by the raw binding.

The remaining skipped functions are listed in `spec/bindings/ctypes.yml` under
`skipped_functions.expected`, and must stay synchronized with
`datoviz._ctypes._SKIPPED_FUNCTIONS`. Their current dispositions are recorded in
`skipped_functions.dispositions`.


## Decision

Do not bind skipped functions merely to reduce the count. The raw Python layer should expose
functions only when the signature is useful, layout-stable, and safe enough for exact `ctypes`
callers.

Low-level Vulkan/runtime configuration and swapchain/surface by-value helpers are not automatically
important Python API. Leave them skipped unless there is a concrete supported Python workflow that
needs them and a safe binding shape exists.


## Recommended next work

1. Keep the 21-function disposition table in `spec/bindings/ctypes.yml` current.
2. For any changed function, choose one disposition:
   - `emit`: add the record to `layout_records.include` only if the full ABI layout is stable and
     user-facing.
   - `ffi-helper`: add a narrowly named `dvz_ffi_*` helper only when the canonical C function is
     useful from raw Python but unsafe or awkward as a by-value return.
   - `intentional-skip`: keep skipped and document why, especially for Vulkan structs, swapchain
     helpers, or redundant descriptor defaults already covered by existing FFI helpers.
   - `remove-export`: remove or demote the C export if the function is accidental public API.
3. Prefer documenting `intentional-skip` over adding low-level wrappers for:
   - `dvz_device_config`
   - `dvz_gpu_ctx_config`
   - `dvz_surface_capabilities`
   - `dvz_surface_extent`
   - `dvz_surface_preferred_format`
   - `dvz_swapchain_extent`
   - `dvz_window_external_surface_info`
4. Reconsider descriptor/default helpers such as `dvz_material_desc`, `dvz_polygon_desc`, and
   `dvz_scalebar_desc` only if their FFI helper coverage is incomplete or inconsistent.
5. After edits to public headers, exported symbols, binding policy, or binding generators, run:

```sh
just ctypes
just ctypes-check
just ctypes-smoke
just docs-api-check
git diff --check
```


## Guardrails

Keep the distinction between direct raw `ctypes` and the top-level array-aware facade clear:

- raw `ctypes` is exact ABI access, validation, debugging, and advanced FFI work;
- `import datoviz as dvz` is the intended direct-engine Python entry point;
- GSP/VisPy2 owns high-level plotting and Pythonic object APIs.

Do not create broad FFI wrappers for every skipped function. A wrapper is justified only when it
removes a real Python FFI hazard while preserving a meaningful user workflow.
