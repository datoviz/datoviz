# DRP2: eliminate base64 from the in-process execution path (COMPLETED)

**Status: DONE.** All items below have been implemented:

- `DvzDrp2Command.write_buffer` and `write_texture` carry both `const void* data_raw`
  (borrowed; in-process path) and `char* data_base64` (heap-allocated; JSON path) — see
  `src/drp2/_stream.h`.
- `dvz_drp2_stream_write_buffer_bytes` sets `data_raw = data` and `data_base64 = NULL`
  (`src/drp2/stream.c`).
- The vklite runtime prefers `data_raw` and only decodes base64 when it is the only thing
  available (`src/drp2/runtime.c` `_vklite_write_buffer` / `_vklite_write_texture`).
- The JSON serializer encodes `data_raw` to base64 on the fly when `data_base64` is NULL
  (`src/drp2/stream.c`).
- `stream_destroy` only frees `data_base64`; `data_raw` is borrowed.

This document is kept for historical context only; no further work is required.

---

## Problem

`DvzDrp2Command.write_buffer.data_base64` and `write_texture.data_base64` are currently used
for **all** execution paths, including in-process vklite execution.  This is wrong: base64
encoding/decoding is pure overhead in-process — it exists only for JSON wire serialization.

The current commit (a65cd992) worked around the `DVZ_MAX_STRING_LENGTH` truncation bug by
heap-allocating the base64 string, but left the encode/decode roundtrip in place.

## Rule

> **Base64 is only ever computed during JSON serialization.  Raw `const void*` data pointers
> are used for everything else.  The vklite runtime must never decode base64 from in-process
> streams.**

## Fix approach

Store `const void* data_raw` in the command struct alongside `char* data_base64`:

```c
struct {
    uint64_t buffer_id;
    uint64_t offset;
    uint64_t size;
    char*       data_base64; /* populated only when serialising to JSON; heap-allocated */
    const void* data_raw;    /* populated for in-process execution; NOT owned — caller keeps it alive */
} write_buffer;
```

(Same pattern for `write_texture`.)

**Emit side** (`dvz_drp2_stream_write_buffer_bytes`):
- Set `data_raw = data` (raw pointer, no copy)
- Set `data_base64 = NULL` (lazily populated on JSON serialization, or never)

**Runtime side** (`_vklite_write_buffer`):
- If `data_raw != NULL`: upload directly, no decode step
- Else fall back to `_decode_base64_exact(data_base64, ...)` (for streams parsed from JSON)

**JSON serializer** (`_json_append_command` / `dvz_drp2_stream_to_json`):
- If `data_raw != NULL`: encode to base64 on the fly and embed in JSON
- Else use `data_base64` directly

**Stream destroy**:
- Free `data_base64` if non-NULL (heap-allocated)
- Do NOT free `data_raw` (borrowed pointer)

## Files to touch

| File | Change |
|------|--------|
| `src/drp2/_stream.h` | Add `const void* data_raw` to `write_buffer` and `write_texture` |
| `src/drp2/stream.c` | `write_buffer_bytes`: set `data_raw`, skip base64; `stream_destroy`: only free `data_base64` |
| `src/drp2/runtime.c` | `_vklite_write_buffer` / `_vklite_write_texture`: prefer `data_raw` |
| `src/drp2/stream.c` | JSON serializer: encode `data_raw` to base64 when needed |
