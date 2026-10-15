# Compact GUI layout helpers

Status: implemented curated C API, with generated Python bindings.

The compact layout helpers operate in a single `DvzGui` context and its existing host window. They do not introduce popup, renderer, docking, or frame-loop ownership.

## Coordinates and sizing

`dvz_gui_current_window_rect()` reports the current ordinary window rectangle in host-window logical pixels. `dvz_gui_content_avail()` reports remaining content width and height in the same units, including inside a child region. Native ImGui coordinates are divided by the GUI coordinate scale exactly once. The active font and user scale affect the native layout before this conversion; callers do not apply them again to a measured distance.

`dvz_gui_same_line()` converts explicit positive offsets and nonnegative gaps from logical pixels; zero offset and negative default-gap sentinels retain native semantics.

`dvz_gui_set_next_item_width()` converts a logical width to native coordinates exactly once. Positive widths specify a fixed distance. Zero selects ImGui's default width. Negative values specify a logical distance back from the available right edge; `-1` leaves one logical pixel. Explicit distances are independent of the active font and user scale. `dvz_gui_input_text_with_hint()` borrows the label, hint and mutable fixed-capacity UTF-8 buffer for the call, using the same value-change and capacity semantics as `dvz_gui_input_text()`.

## Ordinary tool windows

`dvz_gui_set_next_window_rect()` accepts a borrowed finite logical rectangle with positive extents. Requested sizes respect the native minimum and are bounded to the host display; the position is then clamped so the complete rectangle fits. A display smaller than the native minimum rejects the request. The supported Dear ImGui conditions are `0`/`1` (Always), `2` (Once), `4` (FirstUseEver), and `8` (Appearing); combinations and unknown bits are rejected. Once and FirstUseEver preserve subsequent user placement. An explicit pending size, including a raw native request, takes precedence over `DvzGuiConfig.default_window_width`.

These windows use `dvz_gui_begin()` and `dvz_gui_end()`, optionally passing the ordinary ImGui NoDocking flag. Every begin requires an end even when the body is invisible. The optional borrowed open flag controls the existing close affordance. Applications choose anchors, requested rectangles, visibility, and dismissal; no popup or modal semantics are inferred. Applications can use available content and host dimensions to leave room beside a sidebar. Retained widget draw validation follows the actual current native window, including after a nested window ends.

## Icon controls and retained tree density

`dvz_gui_icon_button()` uses ordinary native button interaction and navigation, a stable caller-supplied identifier, and a built-in vector glyph. The glyph set covers clear, settings, information, expand, collapse, multi-selection, ellipsis, and sort. No assets, icon fonts, or textures are retained. A zero extent follows the active font and frame padding; a positive extent is a logical distance independent of font/user scaling. ACTIVE retains selected appearance. DISABLED prevents activation, including a native keyboard intent queued before the button became disabled during the same frame. Native disabled scopes are respected as well. Optional borrowed tooltips appear immediately on hover or native item focus, so on-demand hosts do not require a timer to reach a delayed hover threshold.

Keyboard navigation is opt-in through `DVZ_GUI_FLAGS_NAV_KEYBOARD`; the existing default configuration keeps its input behavior. Icon buttons participate in ordinary Tab navigation and Space activation when enabled. A newly appearing native window may automatically focus its first navigable item, which can also show that item's tooltip.

`DvzGuiTreeLayout.flags` accepts EXACT_SPACING and READABLE_SECONDARY independently. EXACT_SPACING uses the requested font-relative row padding and item spacing without imposing the global style floors. READABLE_SECONDARY uses the row's ordinary foreground for secondary labels, including an explicit row foreground style; missing-value annotations retain their existing dim treatment. Disabled rows retain native disabled alpha. Zero flags preserve the previous layout and color defaults. These scoped changes restore the surrounding native style after drawing.

## Validation

The CPU-only `test_gui_compact_layout` covers coordinate scales one/two crossed with user scales one/one-and-a-half, measured content inside ordinary and child windows, fixed/default/negative widths, hinted-input capacity, default versus explicit placement, bounded placement, Once placement, invalid requests, and balanced visible/collapsed scopes. `test_gui_compact_icons` covers the same four scale profiles, all vector glyphs, stable IDs, logical same-line gaps and offsets, zero-size font-relative geometry, mouse activation, immediate hover help, disabled hover help, real Tab/Space navigation, and same-frame disable transitions. `test_gui_compact_tree_layout` checks exact spacing, restored style, readable secondary text, and unchanged missing-value styling. The isolated GUI configuration test checks default-off and explicit-on keyboard navigation; existing isolated GUI viewport tests cover window input routing.
