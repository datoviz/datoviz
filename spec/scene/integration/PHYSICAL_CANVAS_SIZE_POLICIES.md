# Physical Canvas Size Policies

Status: accepted for v0.4 development.

Datoviz view sizing uses four distinct spaces:

- canvas/reference px: authored scene layout and visual `_px` units;
- host logical px: OS/toolkit window units;
- framebuffer px: GPU/device pixels;
- physical units: display millimeters/inches, approximate for live windows.

The public C contract is:

- `DvzViewSizePolicy`
- `DvzViewSizeDesc`
- `DvzResolvedViewSize`
- `dvz_view_size_desc_framebuffer_px()`
- `dvz_view_size_desc_host_logical_px()`
- `dvz_view_size_desc_reference_px()`
- `dvz_view_size_desc_physical_mm()`
- `dvz_view_size_resolve()`
- `dvz_view_resolved_size()`

`reference_dpi` is the user-facing knob for how physically large a reference pixel should be. The default is `96`.

Screen-space visual attributes are authored in canvas/reference px. Runtime lowering converts them to framebuffer/device pixels through the resolved scale. `user_scale` remains a styling multiplier and must not alter the requested window, canvas, framebuffer, or physical extent.

`DVZ_WINDOW_SIZE_SCALE` is intentionally removed. It scaled only the host window size and did not define a complete canvas-to-framebuffer contract.
