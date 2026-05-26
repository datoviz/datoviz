# External dependency notes

## Dear ImGui and cimgui

Datoviz builds the GUI layer from the Dear ImGui sources nested under `external/cimgui/imgui`.
Do not add or use a separate top-level `external/imgui` checkout for the build.

The curated Datoviz GUI API is implemented in C++ and may call C++ ImGui symbols directly from
`imgui.h`. The raw `datoviz/imgui.h` escape hatch exposes generated cimgui `ig*` symbols for
advanced C users, but Datoviz does not require every C++ ImGui API used internally to be wrapped by
cimgui.

`dvz_gui_slider_range_float()` and `dvz_gui_slider_range_double()` require the C++ range-slider APIs
from ocornut/imgui#9164:

- `ImGui::SliderFloatRange2()`
- `ImGui::SliderScalarRange2()`

Keep `external/cimgui` and its nested `external/cimgui/imgui` checkout compatible. cimgui should be
generated from the same Dear ImGui version/branch family as the nested ImGui sources, while the
range-slider requirement is checked against `external/cimgui/imgui/imgui.h`.
