# C API

Status: generated reference plus hand-written usage notes.

The v0.4 C API is the primary Datoviz API surface. User-facing examples and tutorials should prefer
the retained scene/app path unless they are explicitly about DRP2, vklite, Vulkan, or another
advanced subsystem.

The exhaustive generated outline is [../api_c.md](../api_c.md). It is generated from parsed public
headers and the binding extraction artifact:

```text
build/bindings/datoviz_api.json
```

Do not hand-edit generated reference output or maintain exhaustive symbol tables in prose.


## Scene Frame Artifacts

Scene emission returns a `DvzSceneFrameArtifact`. The artifact is the only scene emission product:
it owns the immutable DRP2 stream snapshot, frozen payload bytes, and setup/update/frame packet
spans for one emitted frame. Retained scene mutation is legal immediately after artifact creation;
mutations affect later artifacts only.

Use `dvz_scene_frame_artifact_stream()` for native DRP2 execution, `dvz_scene_frame_artifact_json()`
for debug or fixture export, and `dvz_scene_frame_artifact_get_packet()` for encoded packet spans.
Destroy the artifact after all borrowed spans and stream pointers are no longer in use.

```c
DvzSceneFrameArtifact* artifact = dvz_figure_emit_frame(figure, NULL, NULL, NULL);
if (artifact == NULL)
    return -1;

const DvzDrp2CommandStream* stream = dvz_scene_frame_artifact_stream(artifact);
// Native runtimes consume the borrowed stream snapshot before artifact destruction.

char* json = dvz_scene_frame_artifact_json(artifact, "debug_frame");
if (json != NULL)
{
    // Debug or fixture export only; not the browser runtime path.
    dvz_drp2_stream_json_destroy(json);
}

const void* packet = NULL;
const void* arena = NULL;
uint64_t packet_size = 0;
uint64_t arena_size = 0;
bool ok = dvz_scene_frame_artifact_get_packet(
    artifact, DVZ_DRP2_PACKET_KIND_FRAME, &packet, &packet_size, &arena, &arena_size);
if (ok && packet != NULL)
{
    // Packet and arena are borrowed from artifact.
}

dvz_scene_frame_artifact_destroy(artifact);
```

The former raw scene emission functions `dvz_figure_emit()` and `dvz_figure_emit_ex()` are removed
in v0.4. Use `dvz_figure_emit_frame()` and retrieve the artifact projection needed by the caller.


## Generated Coverage

The generated outline covers:

1. public headers under `include/datoviz/`;
2. exported functions and signatures;
3. public structs, unions, enums, constants, and callback typedefs;
4. opaque handles and ownership notes where available;
5. raw `ctypes` availability, skipped symbols, and opaque records where binding policy knows them.
