# DRP2 Trace Recorder and Player Design

## Status

Draft design document for a future Datoviz DRP2 trace recorder/player.

This document describes a recording and playback system that stores timestamped DRP2 command streams rather than framebuffer images. The goal is to support compact, replayable, portable visualization recordings.

This is not a conventional video recorder. It is closer to a rendering command trace / replay format, specialized for Datoviz and DRP2.

---

## Motivation

Datoviz should be able to record an interactive or scripted visualization session without storing rendered frames as images or video.

Instead of storing:

```text
frame_000001.png
frame_000002.png
frame_000003.png
...
```

we store:

```text
timestamp -> DRP2 state changes
timestamp -> DRP2 resource writes
timestamp -> frame execution
timestamp -> presentation event
```

This enables:

- replay in real time,
- replay slower or faster than real time,
- frame stepping,
- deterministic debugging,
- replay on another computer,
- replay with a different backend when the DRP2 subset is portable,
- headless export to PNG sequences or MP4,
- compact static-scene recordings,
- inspection of GPU-level rendering state.

The key design principle is:

```text
DVZR records a timed sequence of semantic DRP2 state changes and frame executions.
Persistent object commands and repeated frame command skeletons are deduplicated.
Dynamic data uploads and presentation timestamps are always preserved.
```

MVP note: the first useful implementation should be a raw linear DRP2 replay path before template
interning or aggressive deduplication. The format should be designed so those optimizations can be
added later without changing the meaning of existing recordings.

---

## Terminology

Suggested format name:

```text
DVZR = Datoviz DRP Recording
```

Suggested extension:

```text
recording.dvzr/
recording.dvzr.zip
```

During development, `.dvzr` should be a directory because it is easy to inspect and debug.

For sharing, the same structure can be packaged as a ZIP archive.

---

## High-Level Architecture

The recorder should sit between the DRP2 producer and the renderer.

```text
Scene / App / Python
        |
        v
   DRP2 command stream
        |
        v
+----------------+----------------+
| DRP2 recorder  | live renderer  |
+----------------+----------------+
```

Recording should not affect the scene API. The recorder observes the DRP2 stream and serializes it.

The player reads the recording and feeds reconstructed DRP2 commands into a renderer.

```text
.dvzr recording
        |
        v
   DRP2 player
        |
        v
   DRP2 renderer
        |
        v
 Vulkan / WebGPU / headless / video export
```

### Layer Decision

The normative recording contract is DRP2-level. A DVZR player should be able to replay from DRP2
commands without depending on Datoviz scene objects.

Scene-level data may still be useful as optional sidecar metadata, for example labels, figure/panel
structure, or provenance. It must not be required for pixel-equivalent playback.

Keyframes follow the same rule: the required keyframe is a DRP2 reconstruction stream that recreates
the live DRP2 state at a point in time. A scene recorder may generate that stream by asking the
retained scene to re-emit a full frame/resource reconstruction, but the stored keyframe should remain
DRP2-level.

---

## What Should Be Recorded

The recording must be self-contained enough to reconstruct the rendered result.

It should record:

- resource creation,
- buffer writes,
- texture writes,
- shader module creation,
- pipeline creation,
- bind group / descriptor creation,
- render and compute pass execution,
- draw and dispatch commands,
- queue submissions,
- presentation timing,
- canvas/surface resize events,
- resource destruction, if applicable.

It should not depend on external assets unless those assets are explicitly declared or embedded.

Potential external dependencies that should be embedded or referenced robustly:

- texture images,
- volume data,
- font atlases,
- shader includes,
- colormap textures,
- mesh data,
- data files used for dynamic uploads.

---

## Core Problem: Per-Frame Encoder Duplication

A naive DRP2 trace will be extremely verbose because every frame typically emits a new command encoder and similar transient commands:

```text
frame 0:
    BeginCommandEncoder
    BeginRenderPass
    SetPipeline
    SetBindGroup
    SetViewport
    Draw
    EndRenderPass
    FinishCommandEncoder
    QueueSubmit
    Present

frame 1:
    BeginCommandEncoder
    BeginRenderPass
    SetPipeline
    SetBindGroup
    SetViewport
    Draw
    EndRenderPass
    FinishCommandEncoder
    QueueSubmit
    Present

frame 2:
    same again...
```

Blindly storing all of this every frame is wasteful.

However, some per-frame commands must never be dropped:

```text
WriteBuffer
WriteTexture
CopyBufferToBuffer
CopyBufferToTexture
CopyTextureToBuffer
resource updates
presentation timestamps
resize events
```

For example, camera matrices, MVP uniforms, animated attributes, viewport uniforms, and time-dependent data are often uploaded every frame via buffer writes. These must be preserved.

Therefore, the recording format should distinguish between:

```text
persistent state commands
always-record dynamic data commands
transient frame execution commands
presentation timing commands
```

---

## Command Classification

### Persistent Commands

Persistent commands create or define long-lived logical objects. They should be recorded once, or again only when their definition changes.

Examples:

```text
CreateBuffer
CreateTexture
CreateTextureView
CreateSampler
CreateShaderModule
CreateBindGroupLayout
CreatePipelineLayout
CreateBindGroup
CreateRenderPipeline
CreateComputePipeline
```

These commands are candidates for content hashing and deduplication.

### Dynamic Commands

Dynamic commands update data or copy data. They must be recorded every time they occur, because they define the changing state of the visualization.

Examples:

```text
WriteBuffer
WriteTexture
CopyBufferToBuffer
CopyBufferToTexture
CopyTextureToBuffer
CopyTextureToTexture
```

Large payloads must be stored as external binary blobs, not inline JSON.

A dynamic command may reference a deduplicated blob if the payload is byte-identical, but the command itself should still be represented in the stream unless it is proven semantically redundant.

### Transient Frame Commands

Transient frame commands describe frame execution. These are often repeated with the same structure every frame.

Examples:

```text
BeginCommandEncoder
BeginRenderPass
EndRenderPass
BeginComputePass
EndComputePass
SetPipeline
SetBindGroup
SetViewport
SetScissor
SetBlendConstant
SetStencilReference
Draw
DrawIndexed
DrawIndirect
DrawIndexedIndirect
DispatchWorkgroups
DispatchWorkgroupsIndirect
FinishCommandEncoder
QueueSubmit
```

These commands should usually be compacted into reusable frame execution templates.

### Timing Commands

Timing commands define playback pacing.

Examples:

```text
Present
Resize
FrameBoundary
Hold
RepeatFrames
```

Presentation timestamps must be preserved. They prevent replay from running in immediate mode at thousands of frames per second.

`Resize` here means canvas/surface/presentation-target resize. It does not imply an in-place DRP2
resource resize command.

### Resource Extent Changes

DRP2 should not need dedicated buffer or texture resize commands for the MVP. A resource extent change
is represented as ordinary DRP2 state:

```text
CreateBuffer/CreateTexture with the new size or extent
WriteBuffer/WriteTexture for the new payload
CreateTextureView/CreateBindGroup as needed for rebinding
Destroy* for old resources when lifetime tracking is explicit
```

This keeps replay backend-agnostic and matches APIs where resource sizes are immutable after
creation. Recorders should treat a changed buffer size or texture extent as a new resource
definition, even if the higher-level scene object is conceptually the same.

For retained scene image fields, the scene may keep the same sampled-field object while changing its
extent. During emission, that must become a DRP2 stream with a fresh texture allocation extent and
updated bindings when the logical texture realization changes. Partial texture writes should carry
both the write-region extent and the full allocation extent so a player can reconstruct the texture
without relying on hidden prior runtime state.

---

## Frame Execution Templates

Repeated command encoder / render pass / draw / submit skeletons should be interned as reusable templates.

Instead of storing this every frame:

```json
{"op":"BeginCommandEncoder","id":1001}
{"op":"BeginRenderPass","id":2001,"encoder":1001}
{"op":"SetPipeline","pass":2001,"pipeline":42}
{"op":"SetBindGroup","pass":2001,"index":0,"bind_group":12}
{"op":"Draw","pass":2001,"vertex_count":1000000,"instance_count":1}
{"op":"EndRenderPass","pass":2001}
{"op":"FinishCommandEncoder","encoder":1001,"command_buffer":3001}
{"op":"QueueSubmit","command_buffers":[3001]}
```

record a reusable template:

```json
{
  "type": "template_define",
  "id": 7,
  "commands": [
    {"op":"BeginCommandEncoder","id":"$encoder"},
    {"op":"BeginRenderPass","id":"$pass","encoder":"$encoder","attachments":"$attachments"},
    {"op":"SetPipeline","pass":"$pass","pipeline":42},
    {"op":"SetBindGroup","pass":"$pass","index":0,"bind_group":12},
    {"op":"Draw","pass":"$pass","vertex_count":"$vertex_count","instance_count":"$instance_count"},
    {"op":"EndRenderPass","pass":"$pass"},
    {"op":"FinishCommandEncoder","encoder":"$encoder","command_buffer":"$command_buffer"},
    {"op":"QueueSubmit","command_buffers":["$command_buffer"]}
  ]
}
```

Then each frame can reference the template:

```json
{
  "type": "frame",
  "t": 0.016667,
  "writes": [
    {"op":"WriteBuffer","buffer":5,"offset":0,"size":64,"blob":"blobs/00000001.bin"}
  ],
  "execute": {
    "template": 7,
    "args": {
      "vertex_count": 1000000,
      "instance_count": 1
    }
  },
  "present": true
}
```

If the template arguments are stable, the frame can be even more compact:

```json
{
  "type": "frame",
  "t": 0.016667,
  "writes": [
    {"op":"WriteBuffer","buffer":5,"offset":0,"size":64,"blob":"blobs/00000001.bin"}
  ],
  "template": 7
}
```

---

## Ephemeral IDs

Command encoders, render passes, compute passes, and command buffers are ephemeral.

The recording should not treat their raw IDs as meaningful persistent state.

During template normalization, ephemeral IDs should be replaced by placeholders:

```text
encoder id       -> $encoder
render pass id   -> $pass0
compute pass id  -> $compute_pass0
command buffer id -> $command_buffer
swapchain view   -> $current_swapchain_view
```

During playback, the player may allocate fresh transient IDs for each frame.

This makes frame templates stable across thousands of frames even though raw encoder IDs differ.

---

## Template Normalization

To detect identical frame skeletons, normalize transient commands before hashing.

Normalization should:

- replace ephemeral IDs with placeholders,
- preserve semantically meaningful persistent object IDs,
- preserve command order,
- preserve draw and dispatch parameters unless promoted to template arguments,
- preserve viewport/scissor values unless promoted to template arguments,
- preserve pass attachment structure,
- normalize current swapchain attachment references.

A normalized frame skeleton can then be hashed.

If the hash is already known, emit only a template reference in the frame record.

If the hash is new, emit a `template_define` record.

---

## What Must Not Be Deduplicated Blindly

The following must not be dropped merely because they look similar:

```text
WriteBuffer
WriteTexture
Copy* commands with changing data
DrawIndirect if the indirect buffer changed
DispatchIndirect if the indirect buffer changed
Resize
Present timestamps
Resource destruction
```

Even if two `WriteBuffer` commands have the same target, offset, and size, their data may differ.

Safe optional optimization:

```text
same WriteBuffer target + same offset + same size + same data hash + same prior target state
```

can be compacted, but this is not required for the MVP.

---

## Presentation Timing and Playback Pacing

The player must not replay as fast as command decoding allows unless explicitly requested.

A frame record should include a presentation timestamp.

Preferred:

```json
{
  "type": "frame",
  "t_present": 1.250000,
  "writes": [],
  "template": 7
}
```

Alternative shorthand:

```json
{
  "type": "frame",
  "t": 1.250000,
  "writes": [],
  "template": 7
}
```

If there is only one timestamp, it should be defined as presentation time.

Replay should use presentation time, not emission time, as the pacing clock.

Pseudo-code:

```c
double wall0 = now();
double rec0 = first_frame_time;

while (playing)
{
    double wall = now();
    double target_t = rec0 + speed * (wall - wall0);

    while (next_frame.t_present <= target_t)
    {
        replay_frame(next_frame);
        next_frame++;
    }

    sleep_until_next_frame_or_ui_event();
}
```

Default playback mode must be paced 1x playback, not immediate mode.

---

## Playback Modes

The player should support explicit playback modes.

```text
paced
    default; respects timestamps

step
    one frame per user action

fast
    ignores timestamps; useful for debugging or benchmarking

fixed-fps
    replay/resample at chosen output FPS

export
    deterministic offscreen rendering for PNG or MP4 export
```

CLI examples:

```bash
dvz-drp-player recording.dvzr
dvz-drp-player recording.dvzr --speed 0.5
dvz-drp-player recording.dvzr --fast
dvz-drp-player recording.dvzr --step
dvz-drp-player recording.dvzr --export-png frames/
dvz-drp-player recording.dvzr --export-mp4 output.mp4
```

Default:

```text
paced 1x
```

---

## Holds and Repeated Static Frames

If the application continues presenting while nothing changes, storing identical frames is wasteful.

If there are no writes, no state changes, and the same execution template is used, the recorder may emit a hold or repeat event.

### Hold Event

```json
{
  "type": "hold",
  "t0": 1.000000,
  "t1": 3.000000
}
```

Meaning:

```text
No new DRP2 commands are needed. Keep displaying the current framebuffer until t1.
```

### Repeat Frames Event

```json
{
  "type": "repeat_frames",
  "t0": 2.000000,
  "dt": 0.016667,
  "count": 120,
  "template": 7
}
```

Meaning:

```text
Replay the same frame execution 120 times at 60 Hz.
```

In many static-scene cases, `hold` is preferable to `repeat_frames`.

---

## File Layout

Recommended v1 layout:

```text
recording.dvzr/
    manifest.json
    stream.jsonl
    blobs/
        00000000.bin
        00000001.bin
        00000002.bin
    shaders/
        00000000.wgsl
        00000001.spv
    thumbnails/
        preview.png
```

Minimal MVP layout:

```text
recording.dvzr/
    manifest.json
    stream.jsonl
    blobs/
```

---

## Manifest

`manifest.json` contains global metadata.

Example:

```json
{
  "format": "datoviz-drp-recording",
  "version": 1,
  "drp_version": "2.0",
  "created_utc": "2026-05-15T00:00:00Z",
  "duration_s": 12.532,
  "timebase": "monotonic_seconds",
  "width": 1920,
  "height": 1080,
  "backend_hint": "vulkan",
  "endianness": "little",
  "compression": "none",
  "static_frame_policy": "preserve_timing",
  "content_hash": null
}
```

Possible fields:

```text
format
version
drp_version
created_utc
duration_s
timebase
width
height
backend_hint
platform
gpu_name
driver_version
datoviz_version
endianness
compression
static_frame_policy
required_features
optional_features
content_hash
```

---

## Stream Format

`stream.jsonl` should use JSON Lines: one JSON object per record.

This is preferable to one huge JSON array because it is:

- streamable,
- append-friendly,
- grep-able,
- easy to partially parse,
- robust to interrupted recordings.

Possible record types:

```text
begin
object_commands
template_define
frame
hold
repeat_frames
resize
keyframe
marker
end
```

Example stream:

```json
{"type":"begin","version":1,"drp_version":"2.0","width":1280,"height":720}
```

```json
{
  "type": "object_commands",
  "commands": [
    {"op":"CreateBuffer","id":1,"size":12000000,"usage":["vertex","copy_dst"]},
    {"op":"CreateShaderModule","id":2,"format":"wgsl","blob":"shaders/000000.wgsl"},
    {"op":"CreateRenderPipeline","id":3,"layout":4}
  ]
}
```

```json
{
  "type": "template_define",
  "id": 7,
  "commands": [
    {"op":"BeginCommandEncoder","id":"$encoder"},
    {"op":"BeginRenderPass","id":"$pass","encoder":"$encoder"},
    {"op":"SetPipeline","pass":"$pass","pipeline":3},
    {"op":"SetBindGroup","pass":"$pass","index":0,"bind_group":8},
    {"op":"Draw","pass":"$pass","vertex_count":"$vertex_count"},
    {"op":"EndRenderPass","pass":"$pass"},
    {"op":"FinishCommandEncoder","encoder":"$encoder","command_buffer":"$command_buffer"},
    {"op":"QueueSubmit","command_buffers":["$command_buffer"]}
  ]
}
```

```json
{
  "type": "frame",
  "t": 0.016667,
  "writes": [
    {"op":"WriteBuffer","buffer":5,"offset":0,"size":64,"blob":"blobs/00000001.bin"}
  ],
  "execute": {
    "template": 7,
    "args": {
      "vertex_count": 1000000
    }
  },
  "present": true
}
```

```json
{
  "type": "hold",
  "t0": 0.016667,
  "t1": 1.000000
}
```

```json
{"type":"end","t":12.532000}
```

---

## Binary Blobs

Large payloads must be stored outside JSON.

Avoid:

```json
{
  "op": "WriteBuffer",
  "id": 1,
  "data": [1, 2, 3, 4]
}
```

Prefer:

```json
{
  "op": "WriteBuffer",
  "buffer": 1,
  "offset": 0,
  "size": 1048576,
  "blob": "blobs/00001234.bin",
  "sha256": "..."
}
```

This applies to:

- `WriteBuffer`,
- `WriteTexture`,
- shader binaries,
- large shader sources,
- font atlases,
- embedded images,
- colormap textures,
- large array payloads.

For v1, sequential blob names are fine:

```text
blobs/00000000.bin
blobs/00000001.bin
blobs/00000002.bin
```

Later, use content-addressed storage:

```text
blobs/sha256/ab/cd/abcdef....bin
```

This deduplicates repeated uploads automatically.

---

## JSON vs Binary Command Stream

For the first version, prefer:

```text
JSONL metadata + external binary blobs
```

This is simple, debuggable, and inspectable.

Later, the same logical format could be encoded as:

- MessagePack,
- CBOR,
- FlatBuffers,
- Cap'n Proto,
- custom binary chunks.

Do not start with a complex binary command format unless JSONL performance becomes a real limitation.

---

## Keyframes and Seeking

Without keyframes, seeking requires replaying from the beginning:

```text
seek to t = 10s
replay all commands from t = 0 to t = 10s
```

This is acceptable for short recordings but not long ones.

With keyframes:

```text
seek to t = 10s
find nearest keyframe <= 10s
restore resource/object state
replay commands from keyframe to t = 10s
```

The format should support keyframes from the beginning, even if the first implementation only writes one keyframe at `t = 0`.

Possible layout:

```text
recording.dvzr/
    manifest.json
    stream.jsonl
    blobs/
    keyframes/
        0000.jsonl
        0001.jsonl
```

A keyframe should contain a complete reconstruction of the DRP2 state needed to continue playback from that point:

- live buffers,
- live textures,
- live texture views,
- live samplers,
- live shader modules,
- live pipelines,
- live bind groups,
- current canvas/swapchain state if relevant.

The keyframe payload should be a DRP2 command stream, not a serialized scene graph. If a scene is
available, it can be used as the producer of the reconstruction stream; the player still consumes
only DRP2.

A keyframe record:

```json
{
  "t": 5.000000,
  "type": "keyframe",
  "path": "keyframes/0001.jsonl"
}
```

---

## Recording Algorithm

At record time:

```text
for each emitted DRP2 command:
    classify command

    if persistent object command:
        hash normalized command
        if unseen or changed:
            write object command

    if dynamic data command:
        always write
        move large payload to blob

    if transient frame command:
        append to current frame skeleton

on submit/present:
    normalize current frame skeleton
    hash it
    if skeleton unseen:
        write template_define
    write frame event:
        timestamp
        dynamic writes
        template id
        changed template args
    clear pending frame
```

Pseudo-code:

```c
for command in drp_stream:
    switch classify(command):
    case PERSISTENT:
        key = stable_object_key(command);
        h = hash_normalized(command);
        if (object_hash[key] != h)
        {
            write_object_command(command);
            object_hash[key] = h;
        }
        break;

    case DYNAMIC_WRITE:
        blob = write_blob(command.data);
        write_pending_frame_write(command_without_inline_data, blob);
        break;

    case TRANSIENT:
        append_to_frame_skeleton(command);
        break;

    case PRESENT:
        template_id = intern_template(frame_skeleton);
        write_frame(timestamp, pending_writes, template_id, template_args);
        clear_pending_frame();
        break;
    }
```

---

## Player Algorithm

At playback time:

1. Load manifest.
2. Read object commands and recreate persistent DRP2 state.
3. Read template definitions.
4. For each frame due according to the playback clock:
   - apply writes,
   - expand frame template with arguments,
   - allocate fresh ephemeral IDs,
   - submit reconstructed DRP2 commands,
   - present at the correct time.

Pseudo-code:

```c
load_manifest();
load_initial_objects();
load_templates();

while (playing)
{
    double target_t = playback_clock();

    while (next_record.t <= target_t)
    {
        if (record.type == FRAME)
        {
            apply_writes(record.writes);
            expand_template(record.template, record.args);
            submit_frame();
        }
        else if (record.type == HOLD)
        {
            hold_current_frame_until(record.t1);
        }
        else if (record.type == RESIZE)
        {
            resize(record.width, record.height);
        }
        next_record++;
    }

    poll_ui();
    sleep_until_next_event();
}
```

---

## Minimal Player GUI

A small GUI is useful even for development.

Essential controls:

```text
Play / pause
Timeline scrubber
Playback speed
Step forward one frame
Step backward one frame
Jump to previous/next keyframe
Current time / duration
Current frame index
FPS
```

Useful developer controls:

```text
show command count for current frame
show resource count
show estimated GPU memory
show blob/cache statistics
show backend warnings
inspect commands for current frame
toggle command categories
export current frame as PNG
export full recording as PNG sequence
export full recording as MP4
```

Possible implementations:

### Native Datoviz/ImGui Player

Long-term preferred option:

```bash
dvz-drp-player recording.dvzr
```

It opens a Datoviz window and uses ImGui controls.

### Python Prototype

Good MVP option:

```python
import datoviz as dvz

dvz.play("recording.dvzr")
```

This could use existing Datoviz GUI functionality or minimal keyboard controls at first.

### WebGPU Browser Player

Future option:

```text
record once
replay in browser
share interactive scientific recording
```

This requires the recorded DRP2 subset to be WebGPU-portable.

---

## Suggested Recorder API

Possible C API:

```c
typedef struct DvzDrpRecorder DvzDrpRecorder;

DvzDrpRecorder* dvz_drp_recorder_create(const char* path);

void dvz_drp_recorder_write_commands(
    DvzDrpRecorder* rec,
    double timestamp,
    const DvzDrpCommand* commands,
    uint32_t command_count);

void dvz_drp_recorder_present(
    DvzDrpRecorder* rec,
    double timestamp);

void dvz_drp_recorder_close(DvzDrpRecorder* rec);
```

Blob API:

```c
typedef struct DvzDrpBlobRef
{
    char path[256];
    uint64_t size;
    uint8_t sha256[32];
} DvzDrpBlobRef;

DvzDrpBlobRef dvz_drp_recorder_write_blob(
    DvzDrpRecorder* rec,
    const void* data,
    uint64_t size);
```

Higher-level app API:

```c
void dvz_app_record_start(DvzApp* app, const char* path);
void dvz_app_record_stop(DvzApp* app);
```

Python API:

```python
app = dvz.App()
app.record("recording.dvzr")

# build scene
# run app

app.stop_recording()
```

Or as a context manager:

```python
with dvz.record("recording.dvzr"):
    app.run()
```

---

## Suggested Player API

C API:

```c
typedef struct DvzDrpPlayer DvzDrpPlayer;

DvzDrpPlayer* dvz_drp_player_open(const char* path);
void dvz_drp_player_close(DvzDrpPlayer* player);

void dvz_drp_player_seek(DvzDrpPlayer* player, double t);
void dvz_drp_player_set_speed(DvzDrpPlayer* player, double speed);
void dvz_drp_player_step(DvzDrpPlayer* player);
void dvz_drp_player_play(DvzDrpPlayer* player);
void dvz_drp_player_pause(DvzDrpPlayer* player);
```

CLI:

```bash
dvz-drp-player recording.dvzr
dvz-drp-player recording.dvzr --speed 0.5
dvz-drp-player recording.dvzr --fast
dvz-drp-player recording.dvzr --step
dvz-drp-player recording.dvzr --export-png frames/
dvz-drp-player recording.dvzr --export-mp4 output.mp4
```

Python:

```python
player = dvz.Recording("recording.dvzr")
player.play()
player.seek(5.0)
player.speed = 0.5
```

---

## Determinism Concerns

A DRP2 trace should be more deterministic than a live application, but perfect determinism is not guaranteed.

Potential issues:

```text
random seeds
time-dependent shaders
floating-point differences between GPUs
driver differences
backend differences
undefined resource states
non-portable shader features
external assets
font rasterization differences
```

The recorder should store enough metadata to detect possible replay differences:

```json
{
  "gpu_name": "...",
  "backend": "vulkan",
  "driver": "...",
  "datoviz_version": "...",
  "drp_version": "2.0",
  "portable_subset": true
}
```

The player should warn when replaying on a different backend or unsupported feature set.

---

## Backend Compatibility

A recording made on Vulkan should replay on WebGPU only if it uses the portable DRP2 subset.

The manifest could declare required features:

```json
{
  "required_features": [
    "texture_3d",
    "storage_buffer",
    "timestamp_query"
  ],
  "optional_features": [
    "shader_spirv"
  ]
}
```

The player should validate these before replay.

---

## Compression

For v1:

```text
no mandatory compression
```

Optional per-blob compression:

```json
{
  "blob": "blobs/00001234.bin.zst",
  "codec": "zstd",
  "size_compressed": 123456,
  "size_uncompressed": 1048576
}
```

Recommended later default:

```text
zstd for blobs
possibly zstd-compressed JSONL chunks
```

Avoid mandatory compression in the first prototype.

---

## Error Handling

The recorder should tolerate crashes and interrupted recordings.

Useful conventions:

- write to a temporary directory first,
- append JSONL records incrementally,
- flush periodically,
- write an `end` event only on clean close,
- include checksums for blobs,
- tolerate missing final `end` event during playback.

Example incomplete recording:

```json
{"t":0.000000,"type":"begin","width":1280,"height":720}
{"t":0.016667,"type":"frame","template":7}
```

If there is no `end`, the player should warn but attempt replay.

---

## MVP Scope

### Phase 1: Linear Trace Replay

Implement:

```text
manifest.json
stream.jsonl
external blobs
raw DRP2 command records in emission order
large payloads stored as blobs
presentation timestamps
paced 1x replay
as-fast-as-possible replay mode
```

No arbitrary seeking beyond restart from beginning.

Do not require persistent-object deduplication, frame template interning, content-addressed blobs, or
keyframes in this phase. The success criterion is that a captured DRP2 stream can be played back
linearly and produce the expected frames.

### Phase 2: Trace Compaction

Add:

```text
command classification
persistent object command deduplication
frame template interning
optional blob deduplication
hold intervals for unchanged static frames
```

### Phase 3: Playback Controls

Add:

```text
pause
resume
speed control
frame stepping
basic GUI
```

### Phase 4: Keyframes

Add:

```text
periodic full snapshots
seek support
timeline scrubber
jump to keyframe
```

### Phase 5: Export

Add:

```text
PNG sequence export
MP4 export
headless rendering
fixed-FPS resampling
```

### Phase 6: Optimization

Add:

```text
content-addressed blobs
zstd compression
portable-subset validation
binary command stream encoding
recording chunking
```

---

## Open Design Questions

- Which optional scene-level sidecar metadata is worth storing for inspection without making it
  required for playback?
- Should input events be recorded too, so an interactive session can be replayed from user interaction rather than only from emitted DRP2 commands?
- Should the player allow camera override, or must replay be exact?
- Should the trace include screenshots/thumbnails for quick preview?
- How frequently should keyframes be written?
- How should resource destruction and ID reuse be represented?
- Should shader compiler outputs be recorded, or should shaders be recompiled on playback?
- Should recordings be guaranteed portable across backends, or only best-effort?
- Should the format support streaming over network?
- How should frame template arguments be inferred automatically?
- Should static frames be represented as repeated frames or `hold` intervals?

---

## Optional Extension: Input Event Recording

A DRP2 trace records the rendered command stream, not the original user interactions.

For debugging interaction logic, it may also be useful to record input events:

```json
{
  "t": 1.234,
  "type": "input",
  "event": {
    "kind": "mouse_move",
    "x": 512,
    "y": 300,
    "dx": 4,
    "dy": -2,
    "buttons": 1,
    "modifiers": 0
  }
}
```

This would enable two replay modes:

```text
command replay:
    deterministic rendering replay from DRP2 commands

event replay:
    re-run the scene/application logic from input events
```

For the initial implementation, command replay is simpler and should come first.

---

## Summary

The proposed feature is a DRP2 trace recorder/player.

It should:

- record timestamped DRP2 activity,
- deduplicate persistent object commands,
- deduplicate repeated per-frame command encoder/render pass/draw skeletons using templates,
- preserve all dynamic buffer and texture writes,
- preserve presentation timestamps,
- avoid accidental immediate-mode replay at thousands of FPS,
- store large binary payloads separately from JSON,
- support real-time and variable-speed replay,
- eventually support seeking via keyframes,
- include a small GUI player,
- allow later export to PNG sequences or MP4.

Minimal useful format:

```text
recording.dvzr/
    manifest.json
    stream.jsonl
    blobs/
```

Minimal useful runtime architecture:

```text
DRP2 producer
    -> DRP2 recorder
    -> DRP2 renderer

DVZR file
    -> DRP2 player
    -> DRP2 renderer
```

This would make Datoviz capable of portable scientific visualization capture and replay, which is much richer than ordinary video recording.
