# DRP2 Fixtures

This directory is reserved for canonical conformance traces.

The first fixture set should stay intentionally small:

1. hello triangle
2. indexed draw
3. buffer upload then draw
4. texture upload then sample
5. invalid command ordering
6. unsupported capability rejection

Fixtures should be backend-agnostic and reusable by both native and browser runtimes.

See `FORMAT.md` for the intended structure and naming rules.
