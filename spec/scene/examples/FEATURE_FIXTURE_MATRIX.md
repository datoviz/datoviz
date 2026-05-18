# Datoviz v0.4 Feature Fixture Matrix

> **Status:** Informative planning
> **Scope:** minimal one-feature C examples, generated fixtures, and advanced low-level examples
> **Goal:** maintain one small example idea per implemented or planned Datoviz feature

This matrix is an implementation guide for building a compact example suite. It does not define
runtime semantics. It records what each fixture should demonstrate, what layer it should use, and
which optional capabilities it requires.

Release staging is not decided in this file. Use
[EXAMPLE_RELEASE_STAGING.md](EXAMPLE_RELEASE_STAGING.md) to decide whether a fixture or worked
example is `v0.4 required`, `v0.4 experimental`, `v0.4 fixture-only`, `v0.5`, `later`, or
`external/GSP`. This matrix should stay focused on small, one-feature validation artifacts.

## Principles

1. **One feature per fixture.** Avoid combining multiple visual families, controllers, effects, GUI
   panels, and data loaders in one file unless the feature is the combination itself.
2. **Prefer deterministic offscreen output.** A PNG, JSON stream, DVZR directory, or stdout assertion
   is better than a live window when the feature does not require live interaction.
3. **Use tiny synthetic data.** Prefer three points, one triangle, a 4x4 image, or a tiny 3D scalar
   field over external datasets.
4. **Make optional dependencies explicit.** GLFW, GUI/cimgui, CUDA/NVENC, Kvazaar, WebGPU, and
   external-memory fixtures should be gated in CMake and documented in the fixture row.
5. **Separate fixtures from showcases.** Fixtures prove one API shape; showcase examples may
   combine features into richer applications. A fixture can be release-critical without becoming a
   gallery example.
6. **Keep low-level examples in their own track.** Direct `vk`, `vklite`, `canvas`, `stream`, and
   `drp2` examples are useful, but they target power users and should not obscure the scene examples.

## Output classes

| Output | Use |
| ------ | --- |
| PNG | deterministic visual smoke for offscreen scene/canvas fixtures |
| JSON | scene JSON, DRP2 fixture JSON, or WebGPU-compatible stream fixture |
| DVZR | recording/replay examples and regression inputs |
| MP4/raw video | video encoder/sink fixtures |
| stdout | non-rendering low-level setup, config, and validation examples |
| bounded GLFW window | interaction, GUI, present, or live-sink examples only |

## Scene and app fixtures

| Area | Feature | Status | Example name | Output | Requirements | Minimal fixture idea |
| ---- | ------- | ------ | ------------ | ------ | ------------ | -------------------- |
| Scene lifecycle | Empty scene | Implemented | `fixture_scene_empty.c` | PNG | scene+app | Create one scene, figure, panel, and offscreen app window; capture an empty frame. |
| Figure | Resize | Implemented | `fixture_figure_resize.c` | PNG x2 | scene+app | Render at one size, resize the figure/window, render again. |
| Panel | Layout | Implemented | `fixture_panel_layout.c` | PNG | scene+app | Two panels with different background colors. |
| Panel | Background color | Implemented | `fixture_panel_background.c` | PNG | scene+app | One panel filled by `dvz_panel_set_background_color()`. |
| Visual | Point | Implemented | `fixture_point.c` | PNG | scene+app | Three colored points with different sizes. |
| Visual | Pixel | Implemented | `fixture_pixel.c` | PNG | scene+app | Three screen-space square sprites. |
| Visual | Sphere impostor | Implemented | `fixture_sphere.c` | PNG | scene+app | Three lit spheres with different radii. |
| Visual | Primitive triangle | Implemented | `fixture_primitive_triangle.c` | PNG | scene+app | One RGB triangle with `DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST`. |
| Visual | Primitive line | Implemented | `fixture_primitive_line.c` | PNG | scene+app | One line strip or line list. |
| Visual | Mesh | Implemented | `fixture_mesh.c` | PNG | scene+app | One indexed tetrahedron or cube. |
| Visual | Path | Implemented | `fixture_path.c` | PNG | scene+app | One five-point polyline. |
| Visual | Image | Implemented | `fixture_image.c` | PNG | scene+app | One 4x4 RGBA checkerboard on a quad. |
| Visual | Volume slice | Implemented | `fixture_volume_slice.c` | PNG | scene+app | One tiny 3D scalar field rendered as a slice. |
| Visual data | Initial attributes | Implemented | `fixture_visual_data.c` | PNG | scene+app | Set `position`, `color`, and `size` on one point visual. |
| Visual data | Partial range update | Implemented | `fixture_visual_update_range.c` | PNG x2 | scene+app | Render points, update one item's color/size, render again. |
| Visual data | Many-attribute upload | Implemented | `fixture_visual_data_many.c` | PNG | scene+app | Upload several attributes through the compact multi-data path. |
| Visual data | Mutability hints | Implemented | `fixture_visual_attr_mutability.c` | stdout/PNG | scene+app | Mark size dynamic and position static, then emit one frame. |
| Visual state | Visibility | Implemented | `fixture_visual_visibility.c` | PNG x2 | scene+app | Capture the same scene with a visual hidden and visible. |
| Visual state | Depth test | Implemented | `fixture_depth_test.c` | PNG x2 | scene+app | Two overlapping triangles with depth testing enabled and disabled. |
| Controller | Panzoom | Implemented | `fixture_panzoom.c` | PNG | scene+app | Apply a preset panzoom transform to a 2D path. |
| Controller | Arcball | Implemented | `fixture_arcball.c` | PNG | scene+app | Apply a preset arcball orientation to a cube. |
| Controller | Fly camera | Implemented | `fixture_fly_camera.c` | PNG/live | scene+app | Show a fixed fly-camera pose in a simple 3D scene. |
| Controller | Turntable | Implemented | `fixture_turntable.c` | PNG/live | scene+app | Show a fixed turntable pose around a mesh. |
| Attachment | Fixed overlay | Implemented | `fixture_fixed_overlay.c` | PNG | scene+app | Combine a controlled data visual with a fixed overlay/background. |
| Field | 2D RGBA sampled field | Implemented | `fixture_field_rgba2d.c` | PNG | scene+app | Bind a small RGBA sampled field to an image visual. |
| Field | Scalar field colormap | Implemented | `fixture_field_scalar_colormap.c` | PNG | scene+app | Render an 8x8 float gradient through a continuous scale/colormap. |
| Field | Region update | Implemented | `fixture_field_update_region.c` | PNG x2 | scene+app | Update a 2x2 subregion in an image field. |
| Field | 3D scalar field | Implemented | `fixture_field_scalar3d.c` | PNG | scene+app | Bind a tiny 3D scalar field to a volume visual. |
| Scale | Domain clipping | Implemented | `fixture_scale_domain.c` | PNG | scene+app | Render the same scalar image with a clipped scale domain. |
| Scale | Categorical colormap | Implemented | `fixture_scale_categorical.c` | PNG | scene+app | Render a small label image through a categorical scale. |
| Colorbar | Retained bookkeeping | Implemented bookkeeping | `fixture_colorbar_bookkeeping.c` | stdout/JSON | scene | Create a colorbar object and serialize/inspect it. |
| Colorbar | Rendered colorbar | Planned | `fixture_colorbar_rendered.c` | PNG | scene+app | Render scalar image plus a simple colorbar when rendering lands. |
| Material | Unlit | Implemented | `fixture_material_unlit.c` | PNG | scene+app | One triangle/mesh using unlit material state. |
| Material | Phong/lit | Implemented | `fixture_material_phong.c` | PNG | scene+app | One lit cube or sphere with normals. |
| Effect | Depth cue points | Implemented | `fixture_depth_cue_points.c` | PNG | scene+app | Points at different z values fade/darken with depth. |
| Effect | Depth cue mesh | Implemented | `fixture_depth_cue_mesh.c` | PNG | scene+app | One slanted mesh with depth cue enabled. |
| Technique | MSAA | Implemented | `fixture_msaa.c` | PNG | scene+app | Diagonal line/triangle edges with panel MSAA enabled. |
| Technique | Eye-Dome Lighting | Implemented | `fixture_edl.c` | PNG | scene+app | Dense point cloud with EDL enabled and no GUI panel. |
| Technique | SSAO | Implemented/active | `fixture_ssao_spheres.c` | PNG | scene+app | A few lit spheres with SSAO enabled. |
| Technique | Volume occlusion | Implemented | `fixture_volume_occlusion.c` | PNG | scene+app | Volume plus embedded points/spheres. |
| Transparency | Alpha blending | Implemented | `fixture_alpha_blend.c` | PNG | scene+app | Two overlapping translucent quads. |
| Transparency | WBOIT | Implemented | `fixture_wboit.c` | PNG/live | scene+app | One translucent cube between opaque cards. |
| Transparency | Depth peeling | Implemented | `fixture_depth_peel.c` | PNG/live | scene+app | Two translucent crossing layers. |
| Transparency | Alpha mask | Draft/API-shaped | `fixture_alpha_mask.c` | PNG | scene+app | Checkerboard alpha mask when runtime path is active. |
| Interaction | Point picking | Implemented narrow GPU path | `fixture_point_pick.c` | stdout | scene+app | Queue a pick at a known pixel and print/verify selected item. |
| Interaction | Image probing | Implemented narrow GPU path | `fixture_image_probe.c` | stdout | scene+app | Probe a known texel in a 4x4 scalar image. |
| Interaction | Request coalescing | Implemented | `fixture_request_coalesce.c` | stdout | scene+app | Queue several picks/probes and verify latest-result semantics. |
| Interaction | Selection object | Implemented bookkeeping | `fixture_selection.c` | stdout | scene | Apply a synthetic pick result to a selection and print count. |
| Interaction | Link channel | Implemented bookkeeping | `fixture_link_channel.c` | stdout | scene | Link two panel ranges through a channel with synthetic state. |
| Interaction | Pinned readout | Implemented bookkeeping | `fixture_pinned_readout.c` | stdout | scene | Create a pinned readout from a synthetic probe result. |
| Interaction | Hover picking | Implemented live example | `fixture_pick_hover_glfw.c` | bounded GLFW | scene+app+GLFW | Minimal live hover-pick point scene. |
| Text | Font resource | Implemented bookkeeping | `fixture_font_resource.c` | stdout | scene | Create/destroy a scene-owned font resource. |
| Text | Retained text | Implemented bookkeeping | `fixture_text_bookkeeping.c` | JSON/stdout | scene | Create text, update string/style/placement. |
| Text | Rendered text | Partial/example exists | `fixture_text_rendered.c` | PNG | scene+app | Promote `examples/c/visuals/text.c` behavior into a deterministic offscreen fixture. |
| Annotation | Retained label | Implemented bookkeeping | `fixture_annotation_bookkeeping.c` | JSON/stdout | scene | Create a label annotation and update format. |
| Annotation | Rendered label | Planned | `fixture_annotation_rendered.c` | PNG | scene+app | One point with one label/callout. |
| App | Offscreen render | Implemented | `fixture_app_offscreen.c` | PNG | scene+app | Render one point scene offscreen. |
| App | PNG capture | Implemented | `fixture_capture_png.c` | PNG | scene+app | Render and save a PNG through app/canvas capture. |
| App | GLFW live window | Implemented | `fixture_glfw_window.c` | bounded GLFW | scene+app+GLFW | One point scene, auto-close after N frames. |
| App | Frame callback | Implemented | `fixture_frame_callback.c` | PNG | scene+app | Mutate point color on frame 2 and capture frame 3. |
| App | Hosted window | Implemented smoke | `fixture_hosted_window.c` | bounded GLFW | app+canvas+GLFW | Minimal hosted-window integration smoke. |
| Serialization | Scene JSON | Implemented | `fixture_scene_json.c` | JSON | scene | Serialize one-point scene to JSON. |
| Emission | Figure DRP2 stream | Implemented | `fixture_emit_drp2.c` | JSON | scene+drp2 | Emit a one-triangle frame plan and save debug JSON. |

## GUI and cimgui fixtures

| Area | Feature | Status | Example name | Output | Requirements | Minimal fixture idea |
| ---- | ------- | ------ | ------------ | ------ | ------------ | -------------------- |
| GUI | Overlay attach | Implemented | `fixture_gui_overlay.c` | bounded GLFW | GUI+GLFW | Attach `DvzGui` to a GLFW app window and show one tiny window. |
| GUI | Callback | Implemented | `fixture_gui_callback.c` | bounded GLFW | GUI+GLFW | Register a GUI callback that increments and displays a counter. |
| GUI | Curated controls | Implemented | `fixture_gui_controls.c` | bounded GLFW | GUI+GLFW | One panel with text, button, checkbox, combo, and slider. |
| GUI | Text widget | Implemented | `fixture_gui_text.c` | bounded GLFW | GUI+GLFW | Show one `dvz_gui_text()` item. |
| GUI | Button | Implemented | `fixture_gui_button.c` | bounded GLFW | GUI+GLFW | Button toggles a visual or background color. |
| GUI | Checkbox | Implemented | `fixture_gui_checkbox.c` | bounded GLFW | GUI+GLFW | Checkbox toggles point visibility. |
| GUI | Combo | Implemented | `fixture_gui_combo.c` | bounded GLFW | GUI+GLFW | Combo switches background or alpha mode. |
| GUI | Slider | Implemented | `fixture_gui_slider_float.c` | bounded GLFW | GUI+GLFW | Slider controls point size. |
| GUI | Formatted slider | Implemented | `fixture_gui_slider_float_format.c` | bounded GLFW | GUI+GLFW | Slider controls mesh alpha with custom display format. |
| GUI | Monospace font | Implemented | `fixture_gui_mono_font.c` | bounded GLFW | GUI+GLFW | Display one normal and one monospace line. |
| GUI | Demo window | Implemented | `fixture_gui_demo.c` | bounded GLFW | GUI+GLFW | Show the Dear ImGui demo window; developer-only. |
| GUI | Docking/dockspace | Implemented | `fixture_gui_docking.c` | bounded GLFW | GUI+GLFW | Enable docking and show a small dockable control panel. |
| GUI viewport | Datoviz image viewport | Implemented | `fixture_gui_viewport.c` | bounded GLFW | GUI+GLFW | Render a point figure into a dockable ImGui viewport image. |
| GUI viewport | Input forwarding | Implemented | `fixture_gui_viewport_input.c` | bounded GLFW | GUI+GLFW | Forward viewport input to a panzoom or arcball controller. |
| GUI viewport | Multi-viewport | Implemented | `fixture_gui_multi_viewport.c` | bounded GLFW | GUI+GLFW | Two tiny offscreen figures shown in two GUI viewport windows. |
| cimgui | Raw `ig*` calls | Implemented | `fixture_cimgui_raw.c` | bounded GLFW | GUI+cimgui+GLFW | Include `datoviz/imgui.h` and call one raw cimgui function in a GUI callback. |

## Video fixtures

| Area | Feature | Status | Example name | Output | Requirements | Minimal fixture idea |
| ---- | ------- | ------ | ------------ | ------ | ------------ | -------------------- |
| Video | Encoder defaults | Implemented | `fixture_video_config.c` | stdout | video | Print/default-check codec, mux, backend, size, and fps settings. |
| Video | Sink defaults | Implemented | `fixture_video_sink_config.c` | stdout | video | Build a `DvzVideoSinkConfig` from defaults and print fields. |
| Video | CPU readback config | Implemented | `fixture_video_cpu_readback_config.c` | stdout | video | Configure CPU callbacks for synthetic RGBA frames. |
| Video | Offline/headless encode | Implemented | `fixture_video_offline_encode.c` | MP4 | video backend | Encode 8-16 synthetic RGBA frames to MP4. |
| Video | Builtin stream sink | Implemented | `fixture_video_sink.c` | MP4 | stream+video backend | Register/use `dvz_stream_sink_video()` and feed a few frames. |
| Video | Offscreen scene capture | Planned practical fixture | `fixture_video_scene_offscreen.c` | MP4 | scene+app+video backend | Render a simple offscreen point scene for N frames and encode it. |
| Video | Canvas video sink | Implemented API | `advanced_canvas_video_sink.c` | MP4 | canvas+video backend | Enable `dvz_canvas_configure_video_sink()` on a manual canvas. |
| Video | MP4 streaming mux | Implemented | `fixture_video_mp4_streaming.c` | MP4 | video backend | Encode tiny synthetic frames with streaming MP4 mux. |
| Video | MP4 post mux | Implemented/backend-dependent | `fixture_video_mp4_post.c` | MP4+raw | video backend | Encode to temporary raw stream and post-mux. |
| Video | Raw H.26x stream | Implemented/backend-dependent | `fixture_video_raw_h26x.c` | `.h264`/`.h265` | video backend | Disable MP4 mux and write elementary stream. |
| Video | H.264 | Implemented enum/backend-dependent | `fixture_video_h264.c` | MP4/raw | video backend | Tiny synthetic frames with H.264 codec. |
| Video | HEVC | Implemented enum/backend-dependent | `fixture_video_hevc.c` | MP4/raw | video backend | Tiny synthetic frames with HEVC codec. |
| Video backend | NVENC external capture | Optional | `fixture_video_nvenc_external.c` | MP4 | CUDA+NVENC+Vulkan external memory | Developer-only external image/semaphore encode smoke. |
| Video backend | Kvazaar CPU fallback | Optional | `fixture_video_kvazaar_cpu.c` | MP4 | Kvazaar | Encode CPU RGBA frames through Kvazaar. |
| Video backend | Missing-backend fallback | Implemented internally | `fixture_video_backend_fallback.c` | stdout | video | Request a missing backend and verify graceful fallback/failure. |

## Low-level `vk` fixtures

| Area | Feature | Status | Example name | Output | Requirements | Minimal fixture idea |
| ---- | ------- | ------ | ------------ | ------ | ------------ | -------------------- |
| vk | GPU context | Implemented | `advanced_vk_gpu_context.c` | stdout | vk | Create `DvzGpuCtx`, print selected GPU info, destroy. |
| vk | Validation | Implemented | `advanced_vk_validation.c` | stdout | vk+validation layers | Enable validation and print validation error count. |
| vk | GPU selection | Implemented | `advanced_vk_select_gpu.c` | stdout | vk | Choose GPU index from CLI and print selected device. |
| vk | Feature request | Implemented | `advanced_vk_features.c` | stdout | vk | Request selected Vulkan 1.2/1.3 features before context creation. |
| vk | Instance extension request | Implemented | `advanced_vk_instance_extension.c` | stdout | vk | Add one instance extension to GPU context config. |
| vk | Canvas extensions | Implemented | `advanced_vk_canvas_extensions.c` | stdout | vk+canvas deps | Enable surface/swapchain device extensions for future present canvas use. |
| vk | External-memory allocator policy | Implemented | `advanced_vk_external_memory.c` | stdout | vk+platform support | Configure allocator export handle type for interop. |
| vk | GLSL compilation | Implemented | `advanced_vk_compile_glsl.c` | stdout/SPIR-V | shaderc available | Compile tiny GLSL vertex/fragment/compute string to SPIR-V. |
| vk | Queue access | Implemented | `advanced_vk_queue.c` | stdout | vk | Retrieve graphics/transfer queues from the GPU context. |

## Low-level `vklite` fixtures

| Area | Feature | Status | Example name | Output | Requirements | Minimal fixture idea |
| ---- | ------- | ------ | ------------ | ------ | ------------ | -------------------- |
| vklite | Buffer upload/download | Implemented | `advanced_vklite_buffer.c` | stdout | vk+vklite | Allocate buffer, upload four floats, download and print them. |
| vklite | Buffer resize | Implemented | `advanced_vklite_buffer_resize.c` | stdout | vk+vklite | Create buffer, resize it, verify allocated size. |
| vklite | Image and view | Implemented | `advanced_vklite_image.c` | stdout | vk+vklite | Create a 2D RGBA image and image view. |
| vklite | Image upload | Implemented | `advanced_vklite_image_upload.c` | PNG/stdout | vk+vklite | Upload a 4x4 checkerboard to an image through staging/copy helpers. |
| vklite | Sampler | Implemented | `advanced_vklite_sampler.c` | stdout | vk+vklite | Create nearest and linear samplers. |
| vklite | Shader module | Implemented | `advanced_vklite_shader.c` | stdout | vk+vklite | Compile/load SPIR-V and create a shader module. |
| vklite | Commands | Implemented | `advanced_vklite_commands.c` | stdout | vk+vklite | Allocate command buffer, begin/end/reset/release. |
| vklite | Barriers | Implemented | `advanced_vklite_barriers.c` | stdout | vk+vklite | Record one image layout transition. |
| vklite | Sync | Implemented | `advanced_vklite_sync.c` | stdout | vk+vklite | Create fence/semaphore, submit empty command buffer, wait. |
| vklite | Descriptors | Implemented | `advanced_vklite_descriptors.c` | stdout | vk+vklite | Create a simple uniform-buffer descriptor set. |
| vklite | Graphics pipeline | Implemented helpers | `advanced_vklite_graphics_triangle.c` | PNG | vk+vklite | Draw one triangle without scene or DRP2. |
| vklite | Compute pipeline | Implemented helpers | `advanced_vklite_compute.c` | stdout | vk+vklite | Dispatch a tiny compute shader that writes to a buffer. |
| vklite | Surface wrapping | Implemented | `advanced_vklite_surface.c` | stdout | vk+vklite+GLFW | Wrap a native window surface and print formats/present modes. |
| vklite | Swapchain acquire/present | Implemented | `advanced_vklite_swapchain.c` | bounded GLFW | vk+vklite+GLFW | Create swapchain, acquire one image, present. |
| vklite | Immediate present | Implemented | `advanced_vklite_immediate_present.c` | stdout/live | vk+vklite+GLFW | Minimal immediate-present loop for FPS diagnostics. |

## Window and input fixtures

| Area | Feature | Status | Example name | Output | Requirements | Minimal fixture idea |
| ---- | ------- | ------ | ------------ | ------ | ------------ | -------------------- |
| Window | Headless window | Implemented | `advanced_window_headless.c` | stdout | window | Create a headless window and poll once. |
| Window | GLFW window | Implemented | `advanced_window_glfw.c` | bounded GLFW | window+GLFW | Create a GLFW window, poll events, auto-close. |
| Window | User data | Implemented | `advanced_window_user_data.c` | stdout | window | Attach a struct pointer to a window and retrieve it. |
| Window | Request-frame loop | Implemented | `advanced_window_request_frame.c` | stdout | window | Request frames manually and observe pending state. |
| Window | Surface query | Implemented | `advanced_window_surface.c` | stdout | window+GLFW+vk | Print native surface extent/handle availability. |
| Input | Router callbacks | Implemented | `advanced_input_router.c` | stdout | input | Attach pointer/key callbacks and emit/observe one synthetic event. |

## Stream and canvas fixtures

| Area | Feature | Status | Example name | Output | Requirements | Minimal fixture idea |
| ---- | ------- | ------ | ------------ | ------ | ------------ | -------------------- |
| Stream | Empty stream | Implemented | `advanced_stream_empty.c` | stdout | stream | Create/start/update/stop a stream with a borrowed dummy frame. |
| Stream | Custom sink backend | Implemented | `advanced_stream_custom_sink.c` | stdout | stream | Implement a sink that logs frame ids and timeline values. |
| Stream | Sink registry | Implemented | `advanced_stream_registry.c` | stdout | stream | Register custom sink, find by name, pick automatically. |
| Stream | Multi-sink routing | Implemented | `advanced_stream_multi_sink.c` | stdout/MP4 | stream+optional video | Attach log sink plus video/live sink. |
| Stream | Borrowed frame descriptor | Implemented | `advanced_stream_borrowed_frame.c` | stdout | stream+vk | Construct `DvzStreamFrame` around externally owned Vulkan handles. |
| Stream | Frame usage flags | Implemented | `advanced_stream_usage_flags.c` | stdout | stream | Demonstrate render-target/copy-src/copy-dst metadata. |
| Canvas | Draw callback | Implemented | `advanced_canvas_draw_callback.c` | PNG/live | canvas+vk | Create canvas, set callback, clear/draw manually. |
| Canvas | Offscreen capture | Implemented | `advanced_canvas_offscreen_capture.c` | PNG | canvas+vk | Draw one color/triangle and save PNG. |
| Canvas | Present | Implemented | `advanced_canvas_present.c` | bounded GLFW | canvas+GLFW | Create GLFW-backed canvas and present N frames. |
| Canvas | Immediate FPS | Implemented | `advanced_canvas_immediate.c` | stdout/live | canvas+GLFW | Present as fast as possible and print timing samples. |
| Canvas | RGBA capture | Implemented | `advanced_canvas_capture_rgba.c` | stdout | canvas | Render offscreen, read RGBA, inspect first pixel. |
| Canvas | PNG capture | Implemented | `advanced_canvas_capture_png.c` | PNG | canvas | Render and save PNG without scene/app. |
| Canvas | Video sink | Implemented | `advanced_canvas_video_sink.c` | MP4 | canvas+video backend | Enable canvas video sink and write a short MP4. |
| Canvas | Live-image sink | Implemented | `advanced_canvas_live_image_sink.c` | stdout | canvas+platform handles | Enable live-image callback and print Vulkan handles. |
| Canvas | Underlying stream | Implemented | `advanced_canvas_stream.c` | stdout | canvas+stream | Retrieve `dvz_canvas_stream()` and inspect/attach custom sinks. |
| Canvas | Input router | Implemented | `advanced_canvas_input.c` | stdout/live | canvas+window | Get `dvz_canvas_input()` and wire custom controls. |
| Canvas | Timing diagnostics | Implemented | `advanced_canvas_timing.c` | stdout | canvas | Render frames and print timing samples. |

## Direct DRP2, DVZR, and WebGPU fixtures

| Area | Feature | Status | Example name | Output | Requirements | Minimal fixture idea |
| ---- | ------- | ------ | ------------ | ------ | ------------ | -------------------- |
| DRP2 | Hello stream | Implemented | `advanced_drp2_hello.c` | stdout/JSON | drp2 | Create stream, append `HelloRenderer`, print command count. |
| DRP2 | Raw triangle | Implemented | `advanced_drp2_triangle.c` | PNG/JSON | drp2+runtime | Direct DRP2 triangle without scene. |
| DRP2 | Validation | Implemented | `advanced_drp2_validate.c` | stdout | drp2 | Build valid/invalid streams and print validation results. |
| DRP2 | vklite runtime execution | Implemented | `advanced_drp2_runtime.c` | PNG/stdout | drp2+vklite | Execute a tiny stream through a vklite runtime. |
| DRP2 | Borrowed canvas frame | Implemented | `advanced_drp2_canvas_frame.c` | PNG/live | drp2+canvas | Attach a canvas frame target and render into it. |
| DRP2 | Readback | Implemented | `advanced_drp2_readback.c` | stdout | drp2+vklite | Copy/write into a buffer and read CPU bytes after execution. |
| DRP2 | External buffer | Implemented | `advanced_drp2_external_buffer.c` | PNG/stdout | drp2+vklite | Register a caller-owned buffer under a DRP2 id. |
| DRP2 | Labels/debug trace | Implemented | `advanced_drp2_labels.c` | stdout/JSON | drp2 | Attach debug labels to ids and serialize/print the stream. |
| DRP2 | Fixture JSON | Implemented | `advanced_drp2_fixture_json.c` | JSON | drp2 | Produce a deterministic DRP2 fixture JSON document. |
| DVZR | Raw DRP2 recording | Implemented | `advanced_drp2_recording.c` | DVZR | drp2 | Record/replay raw DRP2 frames without scene/app. |
| DVZR | Scene app recording | Implemented | `fixture_dvzr_record_replay.c` | DVZR+PNG | scene+app+drp2 | Record one offscreen scene frame, replay it, compare/capture output. |
| DVZR | Raw fallback diagnostics | Implemented | `fixture_dvzr_raw_fallback.c` | stdout/DVZR | drp2 | Developer-only unsupported-command fallback diagnostic. |
| WebGPU | Clear | Planned feasibility | `fixture_webgpu_clear.json` | JSON | WebGPU adapter | Clear-only DRP2 subset. |
| WebGPU | Static point | Planned feasibility | `fixture_webgpu_point.json` | JSON | WebGPU adapter | One static point stream. |
| WebGPU | Primitive | Planned feasibility | `fixture_webgpu_primitive.json` | JSON | WebGPU adapter | One triangle stream. |
| WebGPU | Image | Planned feasibility | `fixture_webgpu_image.json` | JSON | WebGPU adapter | One checkerboard image stream. |
| WebGPU | Depth | Planned after basics | `fixture_webgpu_depth.json` | JSON | WebGPU adapter | Two overlapping triangles with depth. |

## Interop and external-consumer fixtures

| Area | Feature | Status | Example name | Output | Requirements | Minimal fixture idea |
| ---- | ------- | ------ | ------------ | ------ | ------------ | -------------------- |
| Interop | Live image handoff | Implemented | `advanced_interop_live_image.c` | stdout | canvas+platform handles | Canvas live-image callback prints image/view/command buffer/FD handles. |
| Interop | External memory export | Implemented lower-level support | `advanced_interop_external_memory.c` | stdout | vk+platform support | Create export-capable allocation metadata. |
| Interop | CUDA-filled external buffer | Implemented test smoke | `advanced_interop_cuda_buffer.c` | PNG/stdout | CUDA+Vulkan external memory | CUDA fills Vulkan-owned buffer, then render/readback. |
| Interop | External timeline semaphore | Implemented lower-level support | `advanced_interop_timeline_semaphore.c` | stdout | CUDA/Vulkan platform support | Demonstrate synchronization metadata and wait/signal path. |
| Interop | Custom external UI sink | Implemented stream substrate | `advanced_interop_custom_sink.c` | stdout | stream+canvas | Route frames into a custom sink for embedding applications. |

## Deferred and planned scene fixtures

These rows should remain planned until the corresponding runtime behavior is landed and validated.

| Area | Feature | Example name | Minimal future fixture idea |
| ---- | ------- | ------------ | --------------------------- |
| Axes | 2D axes/ticks | `fixture_axes_2d.c` | One path/point plot with x/y axes and ticks. |
| Legends | Legend | `fixture_legend.c` | Two visuals with a simple legend. |
| Text | Rendered glyph atlas | `fixture_text_glyph_atlas.c` | One line of text rendered from an atlas. |
| Annotation | Arrow/callout | `fixture_annotation_callout.c` | One arrow from text label to a point. |
| Picking | Mesh face picking | `fixture_pick_mesh_face.c` | Pick an indexed mesh triangle and report face id. |
| Picking | Rich payload ids | `fixture_pick_payload_ids.c` | Verify stable visual/item/group identity payloads. |
| Technique | Object-id G-buffer | `fixture_gbuffer_object_id.c` | Render object ids to an auxiliary target. |
| Technique | Selected outline | `fixture_selected_outline.c` | Select one object and render an outline. |
| Technique | Dual depth peeling | `fixture_dual_depth_peel.c` | Transparent layered geometry after graph-backed path lands. |
| Technique | Curvature/cavity shading | `fixture_cavity_shading.c` | One lit mesh/sphere with cavity shading. |


## Showcase Pressure Tests

These rows are not one-feature fixtures. They record gallery-scale examples that combine several
features and should be backed by smaller fixtures for the individual capabilities.

| Area | Example | Status | Main pressure | Minimal first slice |
| ---- | ------- | ------ | ------------- | ------------------- |
| Embedding dashboard | `IMAGE_EMBEDDING_LOD.md` | Planned showcase | PD12M image thumbnails, texture-array LOD, panzoom, picking, retained resource reuse. | <=10,000 PD12M items rendered as mean-color rectangles from preprocessed embedding positions. |
| Embedding dashboard | `SEMANTIC_EMBEDDING_ATLAS.md` | Planned showcase | Dense semantic points, label LOD, title search, selection cards, optional query vectors. | <=100,000 Wikivecs articles rendered as colored points with panzoom and stable pick ids. |
| Frame graph | Debug graph dump | `fixture_frame_plan_graph_debug.c` | Emit deterministic graph debug output with typed resources/passes. |
| Descriptor refresh | Resize refresh | `fixture_descriptor_refresh_resize.c` | Repeated resize with stable resource ids and refreshed descriptors. |
| CuPy | Python/CuPy external memory | `fixture_cupy_external_memory.py` | Future Python-facing Vulkan-owned allocation import into CuPy. |

## Recommended implementation order

### Phase 1 — tiny deterministic scene fixtures

1. `fixture_point.c`
2. `fixture_pixel.c`
3. `fixture_path.c`
4. `fixture_primitive_triangle.c`
5. `fixture_mesh.c`
6. `fixture_image.c`
7. `fixture_field_scalar_colormap.c`
8. `fixture_volume_slice.c`
9. `fixture_sphere.c`
10. `fixture_panel_layout.c`
11. `fixture_depth_test.c`
12. `fixture_alpha_blend.c`
13. `fixture_scene_json.c`
14. `fixture_dvzr_record_replay.c`

### Phase 2 — interaction, effects, and app features

1. `fixture_point_pick.c`
2. `fixture_image_probe.c`
3. `fixture_frame_callback.c`
4. `fixture_wboit.c`
5. `fixture_depth_peel.c`
6. `fixture_edl.c`
7. `fixture_ssao_spheres.c`
8. `fixture_volume_occlusion.c`

### Phase 3 — GUI and video

1. `fixture_gui_controls.c`
2. `fixture_gui_viewport.c`
3. `fixture_cimgui_raw.c`
4. `fixture_video_config.c`
5. `fixture_video_offline_encode.c`
6. `fixture_video_scene_offscreen.c`

### Phase 4 — low-level power-user examples

1. `advanced_vk_gpu_context.c`
2. `advanced_vklite_buffer.c`
3. `advanced_vklite_image.c`
4. `advanced_vklite_commands.c`
5. `advanced_canvas_offscreen_capture.c`
6. `advanced_canvas_live_image_sink.c`
7. `advanced_stream_custom_sink.c`
8. `advanced_drp2_triangle.c`
9. `advanced_drp2_canvas_frame.c`
10. `advanced_drp2_recording.c`

### Phase 5 — backend-gated and future fixtures

1. `fixture_video_nvenc_external.c`
2. `fixture_video_kvazaar_cpu.c`
3. `advanced_interop_cuda_buffer.c`
4. WebGPU JSON fixture set
5. rendered colorbar/text/annotation fixtures
6. richer picking and frame-graph fixtures
