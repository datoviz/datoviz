# GUI

Datoviz integrates the [Dear ImGUI C++ library](https://github.com/ocornut/imgui) which allows one to create GUIs directly in a Datoviz canvas, without resorting to GUI backends such as Qt or wx. This reduces the number of required dependencies and allows for easy GUI integration.

!!! note
    Datoviz integrates Dear ImGUI via a git submodule ([fork](https://github.com/datoviz/imgui) in the Datoviz GitHub organization). There's a custom branch based on the `docking` upstream branch, which an additional [patch](https://github.com/martty/imgui/commit/f1f948bea715754ad5e83d4dd9f928aecb4ed1d3) applied to it in order to support creating GUIs with integrated Datoviz canvases.

## Creating a basic GUI

Coming soon!
