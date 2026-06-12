# NVDEC Video Source Plan

Status: future implementation plan. This is not v0.4 RC scope unless explicitly promoted.

This plan adds native video decoding for sampled-field/image workflows without introducing a
scene-level video visual. The public surface should be a low-level video source that produces
frames for existing image visuals, with CPU fallback and an optional NVIDIA NVDEC backend shaped
like the current NVENC encoder interop path.


## Goals

1. Decode ordinary scientific video assets into Datoviz sampled fields or texture resources.
2. Support play, pause, stop, stepping, variable playback rate, fast seek, and exact seek.
3. Keep image rendering on the existing scene resource, frame-plan, DRP2, vklite, and canvas path.
4. Provide a CPU fallback for platforms without hardware decode.
5. Make the NVIDIA path mirror the current NVENC approach: CUDA imports or receives GPU decode
   output, converts/copies into Vulkan-visible image resources, and synchronizes explicitly.
6. Keep Vulkan Video as a future backend, not a requirement for the first NVIDIA proof.


## Non-Goals

1. No `dvz_video()` scene-level convenience visual in the first design.
2. No parallel renderer, swapchain, frame stream, or Vulkan wrapper.
3. No backend-specific scene semantics.
4. No audio playback in the first slice.
5. No promise of true constant-time frame-random access for inter-frame compressed codecs.


## Current Encoding Analogue

The existing NVENC encoder path is not Vulkan Video. It uses CUDA external-memory interop:

1. a Vulkan-owned exportable `VkImage` and semaphore are passed to the encoder;
2. CUDA imports the Vulkan image memory and maps it as a CUDA mipmapped array;
3. CUDA waits on the imported semaphore when needed;
4. CUDA copies the image into a linear RGBA device buffer;
5. a CUDA kernel converts RGBA to NV12;
6. NVENC encodes the NV12 device buffer.

The decode path should follow the same engineering style: dynamic NVIDIA SDK loading, explicit
resource import/export, explicit synchronization, no mandatory CUDA link dependency in core, and a
CPU backend when NVIDIA support is unavailable.


## Public C Surface

First-slice API sketch:

```c
DvzVideoSourceConfig cfg = dvz_video_source_config();
cfg.backend = "auto";                // "cpu", "nvdec", later "vulkan-video"
cfg.output = DVZ_VIDEO_OUTPUT_TEXTURE;
cfg.seek_mode = DVZ_VIDEO_SEEK_PREVIEW_THEN_EXACT;
cfg.decode_queue_size = 3;

DvzVideoSource* src = dvz_video_source_open(scene, "movie.mp4", &cfg);
DvzSampledField* field = dvz_video_source_field(src);

dvz_visual_set_field(image, "field", field);

dvz_video_source_play(src);
dvz_video_source_pause(src);
dvz_video_source_stop(src);
dvz_video_source_seek(src, 12.4, DVZ_VIDEO_SEEK_EXACT);
dvz_video_source_set_rate(src, 0.25);
dvz_video_source_step(src, 1);
dvz_video_source_update(src, app_time);
dvz_video_source_destroy(src);
```

Metadata and diagnostics:

```c
const DvzVideoInfo* info = dvz_video_source_info(src);
const DvzVideoStats* stats = dvz_video_source_stats(src);
```

`DvzVideoInfo` should include duration, resolution, nominal FPS, time base, codec, pixel format,
bit depth, color range, chroma subsampling, stream index, keyframe index availability, and selected
backend. `DvzVideoStats` should include decoded frames, displayed frames, dropped frames, pending
packets, pending decoded frames, last seek latency, last upload/copy latency, and backend fallback
reason.


## Seek And Playback Semantics

Compressed video does not provide arbitrary frame access unless every frame is independently coded.
The source API should expose that reality instead of hiding it.

Seek modes:

| Mode | Behavior |
| --- | --- |
| `DVZ_VIDEO_SEEK_KEYFRAME_FAST` | Land on the nearest preceding keyframe and display it quickly. |
| `DVZ_VIDEO_SEEK_EXACT` | Seek to the preceding keyframe, decode forward, and display the requested timestamp/frame. |
| `DVZ_VIDEO_SEEK_PREVIEW_THEN_EXACT` | Display the keyframe quickly, then replace it with the exact frame when ready. |

Playback rates:

1. positive rates display frames according to video timestamps and may drop decoded frames to keep
   wall-clock sync;
2. rate `0` is paused;
3. negative rates are optional later work because many decoders and containers are forward-oriented;
4. deterministic offline playback may use frame stepping rather than wall-clock pacing.


## Backend Architecture

Add a decoder side next to the encoder side in `src/video`:

```text
include/datoviz/video.h
src/video/source_core.c
src/video/source_backend.h
src/video/source_backend_cpu.c
src/video/source_backend_nvdec.c
src/video/source_backend_stub.c
src/video/source_demux_ffmpeg.c
```

The core source owns:

1. demux state;
2. timeline, seek, and rate policy;
3. decoded-frame queue;
4. sampled-field or texture-resource handle;
5. backend selection and diagnostics.

Backends own:

1. codec capability probing;
2. decode session lifetime;
3. output surface format;
4. GPU/CPU frame transfer into the Datoviz sampled-field resource;
5. backend-specific synchronization.

The scene only observes a dirty sampled field. It does not know whether the producer was CPU,
NVDEC, or Vulkan Video.


## CPU Backend

The CPU backend is the portability baseline:

1. demux compressed packets;
2. decode on CPU;
3. convert to RGBA8 for the first slice;
4. update a dynamic or streaming `SampledField`;
5. lower through normal texture upload commands.

Expected copies per displayed frame:

1. CPU decoder output to CPU RGBA, if the decoder does not already produce the target layout;
2. CPU to GPU texture upload.

This is acceptable for correctness, tests, and moderate 1080p clips. It is not the target for
multiple 4K streams or high-rate HEVC/AV1 playback.


## NVDEC Backend

Preferred first NVIDIA path:

1. demux compressed packets on CPU;
2. feed packets to NVIDIA parser/decoder;
3. let NVDEC produce CUDA-accessible decoded surfaces, typically NV12 or P010;
4. copy or convert the decoded CUDA surface into a Vulkan-owned exportable image imported into
   CUDA, mirroring the current NVENC import direction where practical;
5. signal an imported/exported semaphore or record a timeline value so vklite samples only complete
   frames;
6. mark the sampled field dirty for the current frame.

Output variants:

| Variant | Copies | Notes |
| --- | --- | --- |
| NVDEC surface to CUDA linear NV12, CUDA NV12 to Vulkan RGBA8 | one decode-surface copy plus one conversion/write | Easiest image-shader compatibility. |
| NVDEC surface to Vulkan NV12 planes, shader YUV conversion | one GPU copy, less bandwidth | Better steady-state path, needs multi-plane texture or two sampled fields. |
| NVDEC surface imported directly as sampled external image | zero pixel copies after decode | Later target; depends on driver/API details and capability proof. |

The first implementation should choose reliability over theoretical zero copy: GPU-resident decode,
GPU-resident color conversion, no CPU pixel readback, one Vulkan-visible texture update per
displayed frame.


## Vulkan Image Contract

For the NVDEC analogue, Datoviz should allocate a small ring of Vulkan-owned images:

1. sampled usage for rendering;
2. transfer or storage usage for CUDA/Vulkan interop writes when required;
3. external-memory export flags compatible with CUDA import;
4. one semaphore or timeline value per slot;
5. persistent image identity so bind groups/descriptors can be reused where possible.

The source advances through the ring:

```text
decode packet -> NVDEC output surface -> CUDA copy/convert into slot N
              -> signal slot N ready -> scene/frame plan samples slot N
```

The field revision changes when the visible slot changes. Descriptor churn should be avoided by
either reusing a fixed texture resource with copied content or using a small fixed array of views
with stable binding updates.


## DRP2 And Scene Boundary

First slice:

1. no DRP2 video-decode command;
2. no backend handle in scene visuals;
3. decoded frames enter the runtime as ordinary texture writes, external texture registrations, or
   sampled-field resource revisions;
4. image visuals keep their existing placement, sampling, picking, and query behavior.

Future DRP2 extension points may add external texture registration if the runtime needs to sample a
pre-existing image without normal `CreateTexture`/`WriteTexture` ownership. That should remain a
runtime resource contract, not a video semantic command.


## Feature Example

Add a native-only example once the API exists:

```text
examples/c/features/video_source_player.c
```

The example should use an existing image visual plus an ImGui dialog:

1. open bundled or prepared video asset;
2. play, pause, stop, step forward/back;
3. seek slider with keyframe-fast and exact modes;
4. speed slider;
5. metadata: path, backend, codec, size, FPS, duration, pixel format, bit depth;
6. stats: decoded/displayed/dropped frames, queue depth, seek latency, copy/upload latency;
7. visible fallback diagnostics when NVDEC is unavailable and CPU decode is used.

Manifest tags should include `native-only`, `gui`, `video`, and `cuda` when the NVDEC path is
required. A CPU-only variant may keep `video` without `cuda`.


## Validation Plan

1. Header ABI smoke for `DvzVideoSourceConfig`, `DvzVideoInfo`, and `DvzVideoStats`.
2. CPU decoder fixture using a tiny prepared video with deterministic frame colors or checksums.
3. Seek tests for keyframe-fast and exact modes.
4. Playback-rate scheduler tests independent of wall-clock timing.
5. NVDEC capability probe test that skips cleanly without NVIDIA runtime support.
6. NVDEC external-memory smoke on capable hardware: decode one frame, transfer to Vulkan image,
   render through image visual, read back a pixel/checksum.
7. Long-run churn smoke: seek repeatedly, resize/reopen, destroy while decode queue is active.
8. `git diff --check`, `just test video`, and a targeted native example smoke when hardware is
   available.


## Milestones

1. Spec and public API sketch.
2. CPU source backend with deterministic fixture and image visual integration.
3. Video-player feature example using CPU backend.
4. NVDEC provider skeleton with dynamic CUDA/NVIDIA symbol loading and clean fallback.
5. NVDEC GPU decode to CUDA surface with CPU-visible validation disabled by default.
6. CUDA copy/convert into Vulkan-owned image ring with explicit synchronization.
7. Image visual render/readback proof.
8. Performance counters and example diagnostics.
9. Optional Vulkan Video backend evaluation once the source API is stable.


## Performance Expectations

CPU fallback is expected to be adequate for moderate 1080p assets and tests, but may become the
limiting factor for 4K, high FPS, high-bit-depth, HEVC/AV1, or multiple concurrent streams.

NVDEC should avoid CPU pixel traffic. A practical first implementation still performs GPU-local
copies/conversion. For 4K frames, RGBA8 is about 33 MB per frame and NV12 is about 12 MB per frame.
At 60 FPS, that is roughly 2 GB/s for RGBA traffic or 0.75 GB/s for NV12 traffic before overheads,
which is modest for device memory bandwidth. The real risks are synchronization stalls, descriptor
churn, queue depth, and exact-seek decode bursts.

The implementation should report measured decode, copy/convert, upload/register, and render wait
times instead of relying on static backend claims.
