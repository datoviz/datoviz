# External dependency notes

## Dear ImGui and cimgui

Datoviz builds the GUI layer from the Dear ImGui sources nested under `external/cimgui/imgui`. Do not add or use a separate top-level `external/imgui` checkout for the build.

The curated Datoviz GUI API is implemented in C++ and may call C++ ImGui symbols directly from `imgui.h`. The raw `datoviz/imgui.h` escape hatch exposes generated cimgui `ig*` symbols for advanced C users, but Datoviz does not require every C++ ImGui API used internally to be wrapped by cimgui.

`dvz_gui_slider_range_float()` requires the C++ range-slider APIs from ocornut/imgui#9164:

- `ImGui::SliderFloatRange2()`
- `ImGui::SliderScalarRange2()`

Keep `external/cimgui` and its nested `external/cimgui/imgui` checkout compatible. cimgui should be generated from the same Dear ImGui version/branch family as the nested ImGui sources, while the range-slider requirement is checked against `external/cimgui/imgui/imgui.h`.

The target default-on ImPlot/cimplot extension, paired context ownership, raw native C surface, and declarative docking refactor are governed by [GUI_EXTENSIONS_AND_DOCKING.md](GUI_EXTENSIONS_AND_DOCKING.md). The initial official family is based on cimgui `0e533fd0b70f6add19825bea83b66743d5b8d95b` with nested Dear ImGui `f5f6ca07be7ce0ea9eed6c04d55833bac3f6b50b`, plus cimplot `75a03832860f7832712cb5ad8d6e3ad6b69dd97c` with nested ImPlot `524f9fcd48d76c13fdf94c5ffbba8787a1ff7e39`. Treat the four sources and their reviewed Datoviz fork patches as one pinned dependency family and never introduce a second Dear ImGui implementation.

## RC4 refresh assessment (2026-11-17)

Keep the dependency refresh outside RC3. Update one coherent dependency family per checkpoint and retain rollback pins; a blanket move to every upstream HEAD would invalidate unrelated platform, packaging and visual evidence together.

| Family | Observed baseline | RC4 recommendation |
| --- | --- | --- |
| Vulkan toolchain | Local SDK 1.4.328.1; recorded Windows checks also use 1.4.350.0 | Evaluate the [1.4.363 SDK family](https://www.lunarg.com/lunarg-releases-vulkan-sdk-1-4-363-0/), including platform-specific patches such as the [Windows 1.4.363.1 validation fix](https://vulkan.lunarg.com/doc/view/latest/windows/release_notes.html). Align headers, loader, validation, shaderc/glslc and SPIR-V tools as a tested toolchain, while preserving the Vulkan 1.3 runtime/shader contract. A newer SDK does not require a newer minimum GPU API. |
| GUI | Datoviz's Dear ImGui/cimgui 1.91.9b docking/range-slider fork | Perform this with the existing [ImPlot/docking lane](../../agents/now/GUI_IMPLOT_DOCKING_HANDOFF.md). The audited 1.92.7 family is the known starting point; upstream [1.93.0](https://github.com/ocornut/imgui/releases/tag/v1.93.0) is a candidate for a fresh coherent-family audit, not an isolated cimgui pointer bump. Preserve range sliders, generated C wrappers, texture/font backend behavior, paired contexts and Windows exports. |
| GLFW | Untagged revision in the 3.4 development lineage | Compare the exact current revision with [stable 3.5.1](https://www.glfw.org/download.html); select it for relevant window/input/platform fixes, with Linux/macOS/Windows and embedded-host regression checks. Do not infer age solely from the version macro. |
| cglm | 0.9.6 | Consider [0.9.7](https://github.com/recp/cglm/releases/tag/v0.9.7) as a small separate update, with quaternion, projection, camera and rendering tests. Math fixes can change visible output. |
| MSDF tools | msdf-atlas-gen 1.4.0 and a nested 1.13-lineage msdfgen revision | Keep the atlas generator stable unless a concrete text fix justifies change. Compare the exact nested revision with [msdfgen 1.13.1](https://github.com/Chlumsky/msdfgen/releases/tag/v1.13.1); update the compatible generator family and review regenerated atlas hashes/text rendering together. |
| Kvazaar | 2.3.2 | No immediate upgrade identified. Preserve and retest the existing Windows/pthread integration if a later encoder update is needed. |

Start with an inventory and reproducible toolchain baseline, then the coherent GUI lane, then independently justified GLFW/math/text changes. Preserve at least one build against the previous SDK while validating the new one. Check Vulkan validation diagnostics, shader compilation/ABI, installed shaderc and CMake consumers, source/wheel dependency audits, representative rendering and real platform input/resize behavior. Refresh Python bindings before Python/package validation whenever public headers or bindings change. No dependency or submodule update is authorized or performed by this assessment.
