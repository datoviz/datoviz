# Native Video Workflow And Discoverability

Status: future proposal, unimplemented. Documentation, examples, Python ergonomics, and agent guidance described below are planned changes only.

## Existing Pipeline And Gap

The preferred production path already exists: [video export](../../../../docs/how-to/video-export.md), `examples/c/runtime/video_export.c`, and `dvz_view_capture_start()` / `dvz_view_capture_stop()`. Use the scene renderer and native video sink, including offscreen rendering. Do not build a second encoder or renderer. CPU readback and encoding remain a supported baseline; external GPU encoding is capability-dependent, not a universal zero-copy promise.

The C guide is explicit, but the checked Python facade exposes a convenient single-PNG `capture()` helper without a comparable video convenience or companion example. Screenshot-oriented examples can consequently become a misleading starting point for animation. Dependency-local agent rules alone cannot guide an agent working in a consumer repository.

## Proposed Direction

- Add a thin Python recording context over the existing native capture lifecycle. Resolve view ownership, encoder availability, frame submission, output publication, and exception-safe finalization. Exiting recording must not destroy a caller-owned scene or app. Keep generated low-level bindings authoritative.
- Provide equivalent C and Python examples: retained scene resources, deterministic timeline evaluated at `frame_index / fps`, explicit one-frame stepping, and direct MP4 output. Inspect how stepping and capture submission interact so startup frames, final frames, and asynchronous completion are unambiguous.
- Make the native recipe discoverable from animation, offscreen rendering, screenshot, and Python entry-point documentation. Show native typography, font defaults, colorbars, and overlay placement before export. Clearly distinguish a screenshot convenience that owns teardown from recording into an existing application.
- Add concise repository agent guidance that defaults video requests to native recording and links the runnable recipes. Consumer-facing documentation must stand on its own. A PNG sequence or external compositor remains appropriate for diagnostics or a concrete unsupported requirement; identify that requirement rather than silently adopting a separate production pipeline.
- Document supported dimensions, frame rate, encoder capabilities, failure diagnostics, and headless setup through the canonical export guide. Finish one minimal native CPU path before adding optional GPU acceleration demonstrations. Recording is a native-only integration; browser encoder support is separate work.

## User Workflow

1. Build the scientific scene once with retained data and shared typography. Keep scientific content and presentation annotations separately identifiable.
2. Define a deterministic timeline for camera, slice, feature, color scale, and changing labels.
3. Preview a few representative times and scrub, including the intended smaller website display size. Inspect feature switches, units, centering, and numeric stability before committing to a full recording.
4. Record directly to MP4 using the native capture lifecycle and fixed frame stepping. Validate dimensions, frame count, frame rate, duration, finalization, and representative visual states.
5. Revise presentation and record again using the same scientific inputs. This baseline still redraws scientific frames; avoiding that redraw requires the separate [cached-overlay proposal](CACHED_VIDEO_OVERLAYS.md).

## Acceptance Criteria

A fresh user or agent can find and run a short C or Python offscreen recording without creating a PNG directory. A supported native CI smoke checks decodable output, expected frame count and duration, dimensions, and clean resource teardown. Exercise exceptions during recording, failed encoder initialization, repeated recordings, caller-owned application survival, and first/last-frame correctness. Unsupported environments must explain the missing capability and available supported fallback.

## Open Decisions

Settle the Python facade location and ownership model, deterministic submission semantics, and which minimal smoke runs in standard CI. Keep the existing encoder architecture and export contracts; do not make the cached-overlay project a prerequisite for improving discoverability.
