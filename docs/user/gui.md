# Integrated GUI

Visky integrates the [Dear ImGUI C++ library](https://github.com/ocornut/imgui) developed by Omar Cornut. This library makes it possible to design basic to highly complex graphical user interfaces directly in Visky, without any other dependency such as Qt, wx, or similar. Dear ImGui supports a significant number of features and is improving at a rapid pace.

Visky offers a light C wrapper on top of Dear ImGui for the most basic types of graphical interfaces. Using Dear ImGui directly is possible but requires to use C++.

!!! note
    Dear ImGui has a rapid pace of development. Currently Visky integrates it via a git submodule. It uses the `docking` branch, but with [a patch](https://github.com/martty/imgui/commit/f1f948bea715754ad5e83d4dd9f928aecb4ed1d3) applied to it in order to support creating GUIs with integrated Visky canvases. This version currently lives on a [fork](https://github.com/viskydev/imgui) in the viskydev GitHub organization.

## Basic graphical interfaces

## Prompt

## Using the Dear ImGui API directly (C++)
