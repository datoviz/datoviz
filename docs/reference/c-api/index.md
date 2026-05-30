# C API

Status: pending generated reference.

The v0.4 C API is the primary Datoviz API surface. User-facing examples and tutorials should prefer
the retained scene/app path unless they are explicitly about DRP2, vklite, Vulkan, or another
advanced subsystem.

The exhaustive C API reference should be generated from parsed public headers rather than
maintained by hand. The first candidate source is the binding extraction artifact:

```text
build/bindings/datoviz_api.json
```

That generated outline should cover:

1. public headers under `include/datoviz/`;
2. exported functions and signatures;
3. public structs, unions, enums, constants, and callback typedefs;
4. opaque handles and ownership notes where available;
5. raw `ctypes` availability, skipped symbols, and opaque records where binding policy knows them.

Until that generated reference exists, this page is only a placeholder for the generation plan. Do
not hand-edit generated reference output or maintain exhaustive symbol tables in prose.
