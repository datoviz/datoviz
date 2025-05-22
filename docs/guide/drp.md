# Datoviz Rendering Protocol (DRP)

The **Datoviz Rendering Protocol (DRP)** is a low-level specification intended to describe and execute GPU rendering pipelines in a backend-agnostic way. It is inspired by WebGPU and Vulkan, and is designed to serve as a foundation for flexible, portable, high-performance graphics in scientific applications.

---

## Status

!!! warning

    DRP is not yet documented in the Python API.

The core infrastructure exists in the C backend, but it depends on runtime compilation of GPU shaders (SPIR-V). We are currently evaluating the best way to bundle a SPIR-V compiler into Datoviz while preserving cross-platform compatibility.

Until a robust, lightweight solution is found, DRP will remain experimental and not documented on the Python side.

---

## Goals

DRP is designed to:

- Represent GPU rendering pipelines declaratively
- Support custom visuals and advanced rendering techniques
- Enable fully dynamic shader graphs and GPU computation workflows

In the long term, it will serve as the backend for custom shaders and rendering pipelines.

---

## Roadmap

- ✅ C API foundations (in progress)
- 🔲 Runtime SPIR-V compilation integration
- 🔲 Python bindings and visual-level integration
- 🔲 Shader graph and custom pipeline definition API

---

## See Also

- [Datoviz Architecture](../discussions/ARCHITECTURE.md)
- [Visuals](visuals.md) — current prebuilt visual pipeline

---

This page will be updated once DRP becomes available in user-facing APIs.
