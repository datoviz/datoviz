# Timed Media Sync

Status: design notes and future implementation pressure. This is not v0.4 RC scope unless
explicitly promoted.

This document records the desired shape for synchronized scientific media in Datoviz: video, audio,
events, and numeric signals sharing one experiment time axis. The primary use case is an experiment
where a mouse is filmed, recorded acoustically, and instrumented with time-dependent events or
signals that must remain tightly synchronized during playback and analysis.


## Representative Use Case

A behavioral experiment may contain:

1. a video camera recording the animal;
2. a microphone or ultrasonic acquisition stream recording audio;
3. neural, physiology, motion, or stimulus traces sampled at independent rates;
4. event streams such as TTL pulses, trial boundaries, rewards, licks, or detected behaviors;
5. per-device metadata: timebase, start time, sample rate, drift, dropped frames, and gaps.

The user needs to play, pause, seek, scrub, step, and slow down the experiment while preserving
correct timing between visible video frames, audible audio output, waveform/spectrogram displays,
event rasters, and numeric traces.


## Goals

1. Provide one shared media or experiment clock.
2. Synchronize video frames, audio samples, event timestamps, and numeric signal windows.
3. Treat audio as both data and optional output.
4. Represent offsets, drift, gaps, dropped frames, latency, and timestamp provenance explicitly.
5. Keep Datoviz visuals as ordinary consumers of sampled fields, buffers, and item tables.
6. Allow audio output when scientific review requires hearing the signal.
7. Support deterministic offline stepping for analysis and export.


## Non-Goals

1. Do not turn the scene layer into a media player.
2. Do not put audio playback, device callbacks, or audio timing inside DRP2.
3. Do not require audio, FFmpeg, CUDA, or codec dependencies for ordinary rendering.
4. Do not hide timestamp uncertainty or drift behind a single best-effort playback API.
5. Do not make audio output mandatory for audio-as-data workflows.


## Core Concepts

### `DvzMediaClock`

The media clock owns the current experiment time, playback state, rate, and seek generation.

```c
DvzMediaClock* clock = dvz_media_clock_create();
dvz_media_clock_play(clock);
dvz_media_clock_pause(clock);
dvz_media_clock_seek(clock, 12.345678);
dvz_media_clock_set_rate(clock, 0.25);
double t = dvz_media_clock_time(clock);
```

The public API may use seconds as `double`, but the implementation should keep exact rational
timebases or integer ticks where possible to avoid long-run drift.

Clock modes:

| Mode | Meaning |
| --- | --- |
| `DVZ_MEDIA_CLOCK_MANUAL` | Caller or offline runner advances time deterministically. |
| `DVZ_MEDIA_CLOCK_WALL` | Wall-clock playback drives media time. |
| `DVZ_MEDIA_CLOCK_AUDIO` | Audio hardware output is the playback master. |
| `DVZ_MEDIA_CLOCK_EXTERNAL` | Experiment/acquisition timestamps are authoritative. |

For correct audible playback, `DVZ_MEDIA_CLOCK_AUDIO` is often preferred because the audio device
has a real sample clock. For analysis, export, and reproducible validation,
`DVZ_MEDIA_CLOCK_MANUAL` is preferred.


### `DvzTimedSource`

`DvzTimedSource` is a conceptual interface for streams that map media-clock time to data:

1. video source;
2. audio source;
3. numeric signal source;
4. event source;
5. annotation or epoch source.

Each source owns its own timebase, offset, drift correction, missing-data policy, and interpolation
or sampling policy.

```c
dvz_video_source_set_clock(video, clock);
dvz_audio_source_set_clock(audio, clock);
dvz_signal_source_set_clock(trace, clock);
dvz_event_source_set_clock(events, clock);
```


### Time Mapping

Every source should be able to report how it maps source-local timestamps to experiment time:

```text
experiment_time = source_time * drift + offset
```

The first implementation can start with one offset and one drift factor per source. Later work may
need piecewise drift correction for long acquisitions or clock resets.

Required source metadata:

1. source timebase;
2. start timestamp and experiment-time offset;
3. sample rate or frame timestamp list;
4. drift correction;
5. gap/dropped-frame intervals;
6. interpolation policy;
7. timestamp provenance.


## Audio Model

Audio has two distinct roles:

1. audio decode as data;
2. audio output to a device.

Audio decode as data is useful for waveform views, spectrograms, filtering, event alignment, and
quantitative inspection. Audio output is useful when the sound itself is scientifically meaningful
and must be heard in sync with video and events.

Canonical decoded format should be simple:

```text
float32 PCM
interleaved or planar channel layout declared explicitly
source sample rate plus optional resampled output rate
timestamps in media or experiment time
```

The API should support audio windows independent of playback:

```c
const DvzAudioWindow* win = dvz_audio_source_window(audio, t0, t1);
```

Audio windows may feed path visuals, spectrogram image visuals, or downstream user analysis.


## Audio Output Backend

Audio output should be an optional runtime provider below scene semantics:

```text
DvzMediaClock
  -> DvzAudioSource      # decoded samples and timestamps
  -> DvzAudioOutput      # optional speaker/device playback
```

The audio backend owns device selection, callback lifetime, output format, resampling, buffering,
underrun reporting, and hardware latency estimates. It must not call scene, Vulkan, DRP2, or
allocation-heavy code from the audio callback.

The callback path should be real-time safe:

1. no file I/O;
2. no Vulkan or scene calls;
3. no memory allocation;
4. no blocking locks where avoidable;
5. bounded work per callback.

Typical flow:

```text
decode thread -> PCM decode queue -> output ring buffer -> audio device callback
```

The output backend must report:

1. selected device and format;
2. requested and actual sample rate;
3. device latency;
4. buffered audio latency;
5. estimated audible presentation time;
6. underrun and overrun counts;
7. resampling ratio;
8. drift between requested media time and hardware sample clock.


## Backend Options

Preferred first proof: `miniaudio` as an optional provider.

Reasons:

1. C-first, small, and cross-platform;
2. supports playback and capture;
3. avoids writing CoreAudio, WASAPI, ALSA, PulseAudio, or PipeWire code directly;
4. suitable for a feature example and early validation.

Alternatives:

| Backend | Notes |
| --- | --- |
| PortAudio | Mature and common in scientific/audio software, but an external library dependency. |
| SDL audio | Practical if SDL is already in use, otherwise too broad for audio alone. |
| Platform-native APIs | Best control, highest maintenance burden. |

Core `libdatoviz` should not require any audio backend by default. Audio output failure should be a
provider capability diagnostic, not a Datoviz startup failure.


## Audio-Master Synchronization

When audio output is enabled, the media clock should usually follow the estimated audible sample
time rather than wall-clock time.

Conceptually:

```text
audio_clock_time = submitted_sample_time - estimated_remaining_output_latency
```

Video, events, and signals then follow `audio_clock_time`. Video may drop or skip frames to remain
aligned with audio. Numeric signal windows and event rasters update around the same clock time.

On seek:

1. increment the clock seek generation;
2. stop, drain, or silence output;
3. reset decode queues and output ring buffers;
4. seek demux/decode to the requested source time;
5. prefill a small audio buffer;
6. restart output;
7. discard stale video/audio/signal work from earlier seek generations.


## Video Model

Video sources should follow the separate low-level video source plan. Frames become sampled fields
or texture resources consumed by image visuals.

Seek modes:

| Mode | Meaning |
| --- | --- |
| `DVZ_VIDEO_SEEK_KEYFRAME_FAST` | Display a nearby keyframe quickly. |
| `DVZ_VIDEO_SEEK_EXACT` | Decode forward from a keyframe to the requested timestamp. |
| `DVZ_VIDEO_SEEK_PREVIEW_THEN_EXACT` | Show fast preview first, then replace with exact frame. |

In audio-master playback, the video source should choose the frame whose presentation timestamp best
matches the current audio clock. It may drop frames to keep synchronization.


## Signals And Events

Numeric signals should be exposed as time-windowed buffers:

```text
[t - pre_window, t + post_window] -> sample positions and values
```

Events should be exposed as timestamped item tables:

1. point events: markers or raster ticks;
2. intervals: segment or shaded-span visuals;
3. labeled events: categorical colors and label metadata;
4. current-time cursor: shared segment/annotation across panels.

These are ordinary scene resources updated by the media layer.


## Example Application

Future native-only feature example:

```text
examples/c/features/timed_media_sync.c
```

Suggested layout:

1. top panel: mouse video as an image visual;
2. second panel: audio waveform or spectrogram;
3. third panel: event raster and numeric traces;
4. shared vertical current-time cursor across time-series panels;
5. ImGui controls for play, pause, stop, seek, frame step, playback rate, and sync mode;
6. offset and drift controls per source;
7. audio output device selector when audio provider is available;
8. metadata table for video, audio, events, sample rates, timebases, and offsets;
9. diagnostics for dropped video frames, audio underruns, queue depth, seek latency, and measured
   output latency.

The example should work without audio output by showing audio as data. When the audio provider is
available, it should additionally demonstrate correct audible playback.


## Resource Boundary

The media layer updates Datoviz resources:

| Source | Resource |
| --- | --- |
| video frame | `SampledField` or texture resource |
| spectrogram | `SampledField` |
| waveform or numeric trace | buffer/item table |
| events | marker/segment item table |
| epochs | interval geometry or annotations |
| current time | small parameter block or cursor visual attribute |

Scene visuals remain normal consumers. DRP2 receives ordinary resource updates and draw commands.
Audio output does not emit DRP2.


## Diagnostics

Diagnostics should be first-class because scientific timing failures are subtle.

Required diagnostics:

1. selected clock master;
2. source offsets and drift factors;
3. timestamp gaps and dropped video frames;
4. audio device latency and buffered latency;
5. audio underrun/overrun count;
6. decode queue depth per stream;
7. stale frames discarded after seek;
8. seek latency;
9. resampling ratio;
10. active backend names and fallback reasons.


## Validation

1. Synthetic media fixture with known video frames, audio tones/clicks, and event timestamps.
2. Deterministic offline replay: stepping to a timestamp yields the expected frame, sample window,
   and events.
3. Exact-seek validation against known timestamps.
4. Offset and drift correction tests.
5. Audio-as-data tests independent of audio hardware.
6. Audio-output smoke that skips cleanly when no provider or device is available.
7. Latency measurement strategy for provider-capable machines.
8. Long-run playback smoke with queue, underrun, and dropped-frame diagnostics.


## Open Questions

1. Whether FFmpeg is acceptable as the first demux/decode dependency for media files.
2. Whether miniaudio should be vendored, provider-built, or package-discovered.
3. How exact public timestamps should be represented: `double` seconds, rational pairs, integer
   nanoseconds, or explicit timebase ticks.
4. Whether audio capture should share the same provider later.
5. How much resampling policy belongs in core versus the provider.
6. How to package small deterministic audio/video/event fixtures without bloating the repository.
