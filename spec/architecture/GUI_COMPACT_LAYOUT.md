# Compact GUI layout helpers

Status: implemented curated C API, with generated Python bindings.

The compact layout helpers operate in a single `DvzGui` context and its existing host window. They do not introduce popup, renderer, docking, or frame-loop ownership.

## Coordinates and sizing

`dvz_gui_current_window_rect()` reports the current ordinary window rectangle in host-window logical pixels. `dvz_gui_content_avail()` reports remaining content width and height in the same units, including inside a child region. Native ImGui coordinates are divided by the GUI coordinate scale exactly once. The active font and user scale affect the native layout before this conversion; callers do not apply them again to a measured distance.

`dvz_gui_set_next_item_width()` converts a logical width to native coordinates exactly once. Positive widths specify a fixed distance. Zero selects ImGui's default width. Negative values specify a logical distance back from the available right edge; `-1` leaves one logical pixel. Explicit distances are independent of the active font and user scale. `dvz_gui_input_text_with_hint()` borrows the label, hint and mutable fixed-capacity UTF-8 buffer for the call, using the same value-change and capacity semantics as `dvz_gui_input_text()`.

## Ordinary tool windows

`dvz_gui_set_next_window_rect()` accepts a borrowed finite logical rectangle with positive extents. Requested sizes respect the native minimum and are bounded to the host display; the position is then clamped so the complete rectangle fits. A display smaller than the native minimum rejects the request. The supported Dear ImGui conditions are `0`/`1` (Always), `2` (Once), `4` (FirstUseEver), and `8` (Appearing); combinations and unknown bits are rejected. Once and FirstUseEver preserve subsequent user placement. An explicit pending size, including a raw native request, takes precedence over `DvzGuiConfig.default_window_width`.

These windows use `dvz_gui_begin()` and `dvz_gui_end()`, optionally passing the ordinary ImGui NoDocking flag. Every begin requires an end even when the body is invisible. The optional borrowed open flag controls the existing close affordance. Applications choose anchors, requested rectangles, visibility, and dismissal; no popup or modal semantics are inferred. Applications can use available content and host dimensions to leave room beside a sidebar. Retained widget draw validation follows the actual current native window, including after a nested window ends.

## Validation

The CPU-only `test_gui_compact_layout` covers coordinate scales one/two crossed with user scales one/one-and-a-half, measured content inside ordinary and child windows, fixed/default/negative widths, hinted-input capacity, default versus explicit placement, bounded placement, Once placement, invalid requests, and balanced visible/collapsed scopes. GPU window input routing remains covered by the existing isolated GUI viewport tests.
