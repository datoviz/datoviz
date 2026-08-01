# NULL Default Invariant

Status: implemented v0.4 public API contract. Updated: 2026-08-01.

## Invariant

For every public function documented as accepting a pointer-passed config, descriptor, style, or request as `NULL` for defaults, `NULL` is semantically equivalent to passing the canonical public initializer result. Context inheritance must not depend on pointer presence.

If `NULL` means clear, disable, zero, inherit through a distinct operation, or an omitted output pointer, the API documentation must say so directly rather than calling it “defaults.” Required descriptors reject `NULL`.

## Implemented Corrections

- GUI configuration resolves application font defaults identically for `NULL` and `dvz_gui_config()`.
- Retained text resolves scene font size through the same path for `NULL` and `dvz_text_style()`; zero remains the unresolved inherit sentinel.
- Overlay-card construction resolves the same placement and style defaults for `NULL` and `dvz_overlay_card_desc()`.
- Canvas video sinks resolve stream dimensions and FPS identically for `NULL` and `dvz_video_sink_config()`; zero remains the contextual inherit sentinel.
- Scale, colormap, colorbar, legend, visual attachment, and related annotation paths resolve through their public initializers instead of duplicated literals.

The implementation is covered by focused default-equivalence and contextual-inheritance tests. Public header or initializer changes must keep those tests, generated bindings, and generated reference documentation synchronized.

## Design Rules

1. Public initializer functions return the canonical public default record.
2. `NULL for defaults` entry points delegate to that initializer or a resolver shared with the explicit initializer result.
3. Contextual fields use explicit unresolved sentinels and one resolver.
4. Full descriptors do not silently behave as sparse overrides.
5. Clear, disable, zero, and required-pointer semantics remain distinct and explicitly documented.
6. New or changed pointer-passed records require focused equivalence tests when `NULL` defaults are advertised.
