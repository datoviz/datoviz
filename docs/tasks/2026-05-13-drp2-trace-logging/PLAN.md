# DRP2 Stream Logging Plan

Date: 2026-05-13

## Goal

Make live DRP2 stream logging useful while the app is running:

- normal mode should be compact and show only semantic differences between emitted streams;
- full mode should print a complete human-readable command dump;
- volatile ids such as render-pass ids, command-buffer ids, encoder ids, and submission ids should not
  create noise in normal mode;
- the implementation should stay testable even while the broader build is temporarily broken.

## Current Problems

1. The app trace test currently calls `dvz_drp2_stream_begin_render_pass()` with one extra argument,
   which breaks compilation.
2. Normal mode only compares stream fingerprints. It can suppress unchanged frames, but when a stream
   changes it prints the whole current stream rather than a real diff.
3. Full mode omits fields for many command types and is therefore not a complete dump.
4. The trace fingerprint documentation still claims pass ids are stable fields, even though transient
   pass ids are intentionally ignored.
5. The normal output still exposes too much low-level scheduling plumbing for live debugging.

## Implementation Steps

1. Fix the trace test build break and keep focused tests around volatile transient ids.
2. Add a normalized trace snapshot for app windows. The snapshot should contain compact, stable,
   human-readable lines derived from a DRP2 stream.
3. Diff the current normalized snapshot against the previous one in normal mode and print only added
   and removed lines, with a one-line unchanged status for identical snapshots.
4. Keep transient ids out of normal output. Use per-stream render/compute pass ordinals only when needed
   for grouping.
5. Expand full mode so every DRP2 command variant has meaningful fields in its dump.
6. Replace raw-struct fingerprinting with explicit per-command stable hashing so new struct padding or
   future union fields do not accidentally affect duplicate detection.
7. Update internal trace comments and tests to match the new behavior.
8. Validate with `git diff --check`, a focused app trace test build/run if available, and the narrowest
   practical test target once the known build break is fixed.

