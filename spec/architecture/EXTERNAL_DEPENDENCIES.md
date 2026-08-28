# External dependency notes

## Dear ImGui and cimgui

Datoviz builds the GUI layer from the Dear ImGui sources nested under `external/cimgui/imgui`. Do not add or use a separate top-level `external/imgui` checkout for the build.

The curated Datoviz GUI API is implemented in C++ and may call C++ ImGui symbols directly from `imgui.h`. The raw `datoviz/imgui.h` escape hatch exposes generated cimgui `ig*` symbols for advanced C users, but Datoviz does not require every C++ ImGui API used internally to be wrapped by cimgui.

`dvz_gui_slider_range_float()` requires the C++ range-slider APIs from ocornut/imgui#9164:

- `ImGui::SliderFloatRange2()`
- `ImGui::SliderScalarRange2()`

Keep `external/cimgui` and its nested `external/cimgui/imgui` checkout compatible. cimgui should be generated from the same Dear ImGui version/branch family as the nested ImGui sources, while the range-slider requirement is checked against `external/cimgui/imgui/imgui.h`.

The target default-on ImPlot/cimplot extension, paired context ownership, raw native C surface, and declarative docking refactor are governed by [GUI_EXTENSIONS_AND_DOCKING.md](GUI_EXTENSIONS_AND_DOCKING.md). The initial official family is based on cimgui `0e533fd0b70f6add19825bea83b66743d5b8d95b` with nested Dear ImGui `f5f6ca07be7ce0ea9eed6c04d55833bac3f6b50b`, plus cimplot `75a03832860f7832712cb5ad8d6e3ad6b69dd97c` with nested ImPlot `524f9fcd48d76c13fdf94c5ffbba8787a1ff7e39`. Treat the four sources and their reviewed Datoviz fork patches as one pinned dependency family and never introduce a second Dear ImGui implementation.
