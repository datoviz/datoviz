# Public Struct ABI Audit

Status: current v0.4 pre-freeze structural audit generated from `build/bindings/datoviz_api.json`. Updated: 2026-08-01.

This audit applies [PUBLIC_API_CONVENTIONS.md](PUBLIC_API_CONVENTIONS.md): growable public descriptors and configs passed by pointer begin with `uint32_t struct_size` and `uint32_t flags` and have a canonical initializer.

## Current Structural Result

The generated binding inventory contains 112 non-opaque public records whose names end in `Config`, `Desc`, `Style`, or `Request`. Of those, 107 begin with the required `struct_size`, `flags` prologue.

The five records without that prologue are intentional fixed-layout or low-level records:

| Record | Classification |
| --- | --- |
| `DvzDeviceQueueRequest` | Fixed row describing one queue-family count request. |
| `DvzPanelDesc` | Fixed by-value panel placement record. |
| `DvzShaderCompileRequest` | Fixed compilation input record; the typed API validates its complete contents and does not promise growable descriptor ABI. |
| `DvzStreamSinkRequest` | Fixed stream request row pairing backend and configuration pointer. |
| `DvzSwapchainConfig` | Low-level vklite record retained as a fixed advanced/unstable setup value. |

No remaining naming-matched caller-authored growable descriptor lacks the ABI prologue.

## Records Outside The Naming Scan

Event payloads, result/output records, geometry/data rows, colors and time values, internal containers, borrowed runtime records, DRP2 protocol rows, batch update rows, and backend callback tables do not require a prologue solely because they are public. Protocol versioning, enclosing-count contracts, or fixed-layout semantics govern those records.

Examples include input events, `DvzGpuInfo`, shader compile results, query results, `DvzStreamFrame`, DRP2 bind-group entries and attachments, visual data updates, category/color rows, and backend proc tables.

## Freeze Rule

After any public header or binding-generator change, regenerate `build/bindings/datoviz_api.json` with `just ctypes`, rerun the structural classification, and validate with `just ctypes-check`. Any new pointer-passed growable config/descriptor/style/request must either use the ABI prologue plus initializer or record an explicit fixed-layout rationale here.
