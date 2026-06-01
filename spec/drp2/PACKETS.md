# DRP2 Binary Packets

Status: active break-compatible WebGPU/WASM transport contract draft.

This document defines the browser-facing DRP2 packet boundary that replaces JSON as the runtime hot
path. JSON stays available only for fixtures, debugging, and human-readable evidence.

## Goals

1. Make WASM-to-browser transport a compact binary command packet plus payload arena.
2. Preserve stable DRP2 ids across repeated browser frames.
3. Split scene emission into explicit `setup`, `update`, and `frame` packets.
4. Remove JavaScript command-name heuristics and browser-side mutation of scene-owned resources.
5. Keep the browser runtime generic: it executes DRP2 packets, not scene visual families.

## Packet Result Boundary

A scene emission returns one structured result:

1. `status`: success, validation error, unsupported feature, or internal error;
2. `diagnostics_ptr`, `diagnostics_size`: UTF-8 diagnostics owned by the WASM scene bridge until the
   next mutating bridge call or explicit result destroy;
3. `setup_packet_ptr`, `setup_packet_size`;
4. `update_packet_ptr`, `update_packet_size`;
5. `frame_packet_ptr`, `frame_packet_size`;
6. `payload_arena_ptr`, `payload_arena_size`;
7. `scene_version`, `resource_version`, and `frame_index` counters.

A zero packet pointer or zero size means that phase has no commands. Packet pointers and the payload
arena are borrowed WASM memory; JavaScript must execute or copy them before another mutating bridge
call. Browser runtimes must never retain borrowed WASM spans after result destroy or scene destroy.

## WASM Scene Bridge ABI

The experimental browser scene bridge exposes split packet emission through these C/WASM exports:

```text
int      dvz_wasm_api_emit_packets(scene, figure)
int      dvz_wasm_api_packet_status(scene)
uint32_t dvz_wasm_api_packet_ptr(scene, kind)
uint32_t dvz_wasm_api_packet_size(scene, kind)
uint32_t dvz_wasm_api_packet_arena_ptr(scene, kind)
uint32_t dvz_wasm_api_packet_arena_size(scene, kind)
uint32_t dvz_wasm_api_resource_version(scene)
uint32_t dvz_wasm_api_frame_index(scene)
```

`kind` is the numeric packet phase id: `1` setup, `2` update, `3` frame.

All returned pointers are borrowed WASM linear-memory addresses. They remain valid only until the
next mutating scene bridge call, the next emit call, or scene destruction. JavaScript may create
temporary `Uint8Array` views for immediate decode/upload, but must not retain those views across
another bridge call. JSON emit functions remain a debug/fixture export and are not the browser
runtime path.

Current browser runtime requirements:

1. a packet set must include a `frame` packet;
2. all non-empty phases in one packet set must carry the same `resource_version` and `frame_index`;
3. runtimes reject packet sets with older `resource_version` or non-increasing `frame_index`;
4. reset starts a new retained browser runtime session and clears packet counters.

## Packet Phases

### `setup`

Creates, recreates, and destroys retained GPU resources and dependency objects.

Allowed command families:

1. handshake and capability negotiation needed for the session;
2. create/destroy buffer, texture, sampler, shader, bind-group-layout, bind-group, and pipeline;
3. setup-bearing uploads required to initialize newly created resources.

A setup packet advances `resource_version`. Replaying an older setup packet against a newer retained
session is invalid unless the session has been reset.

### `update`

Mutates retained resources without changing their identities or dependency graph.

Allowed command families:

1. `WriteBuffer`;
2. `WriteTexture`;
3. copy commands used as resource updates;
4. explicit barriers once promoted into the active DRP2 synchronization contract.

Same-shape visual data changes should be update-only. Shape, format, usage, or topology changes that
require new resource identities must emit setup before update/frame replay.

### `frame`

Encodes transient frame work against retained resources.

Allowed command families:

1. command encoder/pass begin/end;
2. dynamic render state;
3. bind pipeline/group/vertex/index state;
4. draw, dispatch, copy, finish, and submit commands;
5. readback request metadata when the target supports it.

Frame packets must not implicitly create resources. A browser runtime must reject unknown ids rather
than infer setup from command names.

## Binary Layout

All multi-byte fields are little-endian. All offsets are byte offsets from the start of the packet or
payload arena as stated. Writers align command records and arena payloads to 8 bytes. Readers must
reject unaligned offsets, overflow, truncated records, unknown required flags, and command bodies
whose declared size does not match the command type.

```text
DvzDrp2PacketHeader
  magic[8]          = "DVP2PKT\0"
  header_size       = bytes from packet start to first record
  version_major     = 2
  version_minor     = 0
  packet_kind       = setup | update | frame
  flags             = 0 for v2.0
  command_count
  command_bytes     = byte size of all command records
  arena_size        = byte size of the companion payload arena
  resource_version
  frame_index

DvzDrp2PacketRecord[command_count]
  command_type      = DvzDrp2CommandType numeric value
  record_flags      = 0 for v2.0 unless defined by the command
  body_size
  payload_offset    = UINT64_MAX if no payload
  payload_size      = 0 if no payload
  body[body_size]   = fixed command fields, no host pointers
  padding           = zero bytes to 8-byte alignment
```

The payload arena is separate from the packet bytes. Payload-bearing commands reference arena spans
with `payload_offset` and `payload_size`. The first v2.0 payload-bearing commands are `WriteBuffer`
and `WriteTexture`; shader bytecode and other large binary inputs must use the same arena mechanism
when promoted. No command body may contain host pointers, JavaScript object handles, base64 strings,
or browser-only metadata.

## Payload Arena Rules

1. Arena offsets are stable only within one emission result.
2. Payload contents are immutable after the result is returned.
3. Multiple commands may reference the same arena span only when the scene emitter deliberately
   deduplicates identical bytes and both commands use the exact same range.
4. The browser runtime copies or uploads from arena spans during packet execution; it must not retain
   the WASM memory view.
5. Base64 payloads are forbidden in runtime packets. They remain valid only in JSON fixtures/debug
   exports.

## Runtime Session Contract

The browser owns a DRP2 runtime session with:

1. WebGPU adapter/device/canvas context and target format;
2. retained DRP2 resource tables keyed by stable ids;
3. packet validation and execution;
4. resize, input normalization, render scheduling, diagnostics, and destroy;
5. resource-version tracking.

Execution order for one browser tick is:

```text
if setup packet exists: validate resource_version and execute setup
if update packet exists: execute update against current retained resources
execute frame packet against current retained resources
```

A failed packet leaves the session in the previous committed state for setup/update phases. A failed
frame packet may discard transient command encoder/pass state but must not mutate retained scene
resources except for uploads already committed before the failure; runtimes should validate before
execution where practical.

## JSON Export

JSON export is a fixture/debug view of the same DRP2 commands. It is not the browser runtime
protocol. New browser features must first be representable in binary packets and the payload arena;
JSON support may then be added for evidence and conformance fixtures.
