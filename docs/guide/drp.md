# Datoviz Rendering Protocol (DRP)

The **Datoviz Rendering Protocol (DRP)** is a low-level specification intended to describe and execute GPU rendering pipelines in a backend-agnostic way. It is inspired by WebGPU and Vulkan, and is designed to serve as a foundation for flexible, portable, high-performance graphics in scientific applications.

The Datoviz Scene API, which includes everything documented on this website, is entirely based on DRP.

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
- Support advanced rendering techniques
- Carry the narrow compute and render command streams used by the v0.4 scene path

In the long term, it can serve as the backend for custom visual families and broader rendering
pipelines. General custom render shaders, built-in shader replacement, and shader hot reload are
deferred from the v0.4 public surface.

---

## See Also

- [Datoviz Architecture](../discussions/ARCHITECTURE.md)
- [Visuals](visuals.md) — current prebuilt visual pipeline

---

This page will be updated once DRP becomes available in user-facing APIs.
